#include <os.h>
#include <math.h>
#include <driver/hrtimer.h>
#include <driver/gpio.h>
#include <driver/irq.h>
#include <sys/times.h>
#include <properties.h>
#include <udisk/usb_detect.h>
#include "flash/flashopt.h"
#define LOG_TAG    "battery"
#define LOG_LVL    ELOG_LVL_VERBOSE
#include <elog.h>

#define BAT_CHARGING                1
#define BAT_DISCHARGING             0

#define BAT_VOLTAGE_MAX             4200
#define BATDETE_TO_VBAT_RES         1000        // 千欧姆
#define BATDETE_TO_GND_RES          332         // 千欧姆

#define ADJUST_WEIGHT               0.2
#define RECORD_COUNT                10
#define PROP_NAME                   "battery"

#define SEC(sec)                    ((sec)*1000*1000LL) // us
#define DEFAULT_INTERVAL            SEC(30)
#define MAX_INTERVAL                SEC(300)


struct capacity_params {
    int capacity;
    int min;
    int max;
    int offset;
    int hysteresis;
};

struct capacity_table {
    int size;                            // 容量表的大小
    int stride;                          // 1% 容量对应的电压差值
    struct capacity_params* cap_params;  // 电池容量参数表
};

struct tv_table {
    uint64_t sysTime;           // system time
    uint32_t instVoltage;       // instant voltage
};

struct battery_info {
    int max_voltage;            // standard max voltage
    int pre_voltage;            // last time voltage
    int pre_percent;            // last time percent
    char battery_mode;          // charge or discharge
    semaphore_t detect_state;
};

/* 需要根据电池厂商的电池曲线做出来 */
static struct capacity_params capParams[]= {
   /*capacity, min, max, offset, hysteresis*/
    {0, 3306, 3426, 0, 1 },
    {1, 3427, 3638, 0, 9 },
    {10,3639, 3697, 0, 10},
    {20,3698, 3729, 0, 10},
    {30,3730, 3748, 0, 10},
    {40,3749, 3776, 0, 10},
    {50,3777, 3827, 0, 10},
    {60,3828, 3895, 0, 10},
    {70,3896, 3954, 0, 10},
    {80,3955, 4050, 0, 10},
    {90,4051, 4119, 0, 10},
    {100,4120,4240, 0, 1 },
};

extern void adc_init(void);
extern void adc_deinit(void);
extern int adc_get_number(void);


static bool mStop = false;
static int table_index = 0;
static int usb_drvvbus = GPIO_PC(24);
static uint8_t usb_power_state;

static struct hrtimer gtimer;
static struct capacity_table capTable;
static struct tv_table tvTable[RECORD_COUNT];
static struct battery_info mybattery;

volatile static uint64_t time_interval = DEFAULT_INTERVAL;

#define FIND_INDEX_BY_BAT(vbat) ({                  \
            int low = 0, mid = 0;                   \
            int high =  capTable.size - 1;          \
            while(high >= low) {                    \
                mid = (low + high) / 2;             \
                if(vbat < capParams[mid].min)       \
                    high = mid - 1;                 \
                else if(vbat > capParams[mid].max)  \
                    low = mid + 1;                  \
                else                                \
                    break;                          \
            }                                       \
            mid;                                    \
        })


/*
 *  ABOUT USB FUNC
 */
static void __unused usb_drvvbus_irq_handle(int irq, void* data)
{
    uint8_t usb_power_state = gpio_get_value(usb_drvvbus);
    mybattery.battery_mode = usb_power_state;
    time_interval = DEFAULT_INTERVAL;
    semaphore_post(&mybattery.detect_state);
}

static void udisk_battery_callback(int data)
{
    usb_power_state = data ? BAT_CHARGING : BAT_DISCHARGING;
    if(usb_power_state != mybattery.battery_mode) {
        mybattery.battery_mode = usb_power_state;
        time_interval = DEFAULT_INTERVAL;
        semaphore_post(&mybattery.detect_state);
    }
}

static void usb_detect_init(void)
{
#if 0
    int ret = gpio_request(usb_drvvbus, "usb_drvvbus");
    if(ret) {
        log_e("Failed to request drvbus pin\n");
        if(usb_drvvbus != -1)
            gpio_release(usb_drvvbus);
    }
    gpio_direction_input(usb_drvvbus);
    mybattery.battery_mode = gpio_get_value(usb_drvvbus);
    request_irq(gpio_to_irq(usb_drvvbus), IRQ_TYPE_EDGE_BOTH, usb_drvvbus_irq_handle, "usb_drvvbus", NULL);
#endif
    register_usb_callback(udisk_battery_callback);
}

static void __unused usb_detect_deinit(void)
{
    disable_irq(gpio_to_irq(usb_drvvbus));
    release_irq(gpio_to_irq(usb_drvvbus));
    gpio_release(usb_drvvbus);
}


/*
 *  ABOUT HRTIMER FUNC
 */
static void hrtimer_cb(struct hrtimer *hrtimer)
{
    semaphore_post(&mybattery.detect_state);
    hrtimer_restart(hrtimer, time_interval);
}


/*
 *  ABOUT BATTERY VOLTAGE DETECT FUNC
 */
static int get_battery_voltage(void)
{
    int adcNum = adc_get_number();
    return (adcNum * (BATDETE_TO_GND_RES + BATDETE_TO_VBAT_RES) / BATDETE_TO_GND_RES);
}

static void battery_record(int vbat)
{
    char battery[16] = {0};
    sprintf(battery, "%d", vbat);
    ds_write("battery", battery, strlen(battery));

#ifdef AUTO_TEST
    FILE* fp = fopen("/data/battery", "a");
    fprintf(fp, "%d\n", vbat);
    fclose(fp);
#endif
}

static int calculate_battery_percent(int vbat);
static void get_battery_before_poweroff(void)
{
    char value[4] = {0};
//    get_property(PROP_NAME, value, "-1");
    ds_read(PROP_NAME, value, 4);

    struct capacity_params* tb = capTable.cap_params;
    if(atoi(value) <= 0 || atoi(value) > 100) {
        mybattery.pre_voltage = get_battery_voltage();
        mybattery.pre_percent = calculate_battery_percent(mybattery.pre_voltage);
    }
    else {
        mybattery.pre_percent = atoi(value);
        if(!strncmp(value, "0", strlen(value)))
            mybattery.pre_voltage = tb[0].max;
        else if(!strncmp(value, "100", 3))
            mybattery.pre_voltage = tb[capTable.size - 1].min;
        else {
            for(int i=0; i<capTable.size - 1; i++) {
                if(atoi(value) >= tb[i].capacity && atoi(value) <  tb[i+1].capacity) {
                    mybattery.pre_voltage = tb[i].min + atoi(value)%(tb[i].hysteresis)*(tb[i].max - tb[i].min)/tb[i].hysteresis;
                    break;
                }
            }
        }
    }
}

static void battery_params_init(void)
{
    memset(&tvTable, 0, sizeof(struct tv_table));
    memset(&capTable, 0, sizeof(struct capacity_table));
    memset(&mybattery, 0, sizeof(struct battery_info));

    mybattery.max_voltage = BAT_VOLTAGE_MAX;
    mybattery.battery_mode = BAT_DISCHARGING;
    semaphore_init(&mybattery.detect_state,0);

    capTable.cap_params = capParams;
    int size = sizeof(capParams) / sizeof(struct capacity_params);
    int stride = (capParams[size-1].min - capParams[0].min) / 100;
    capTable.size = size;
    capTable.stride = stride;
    usb_power_state = gpio_get_value(usb_drvvbus);
}

static int calculate_battery_voltage(int vbat)
{
    int diff = 0;
    int adjust_voltage = 0;

    if(table_index >= RECORD_COUNT) {
        struct tv_table* limit_table[2];
        /* find max voltage、min voltage in array sequence and calculate time count */ 
        limit_table[0] = limit_table[1] = &tvTable[0];
        for(int i=0; i<RECORD_COUNT; i++) {
            if(tvTable[i].instVoltage < limit_table[0]->instVoltage)
                limit_table[0] = &tvTable[i];
            else if(tvTable[i].instVoltage >= limit_table[1]->instVoltage)
                limit_table[1] = &tvTable[i];
        }
        int tinterval = abs(limit_table[1]->sysTime - limit_table[0]->sysTime);
        int vinterval = limit_table[1]->instVoltage - limit_table[0]->instVoltage;
        if(vinterval != 0) {
            time_interval = tinterval * capTable.stride * 1000 / vinterval;
            if(time_interval <= 0)
                time_interval = DEFAULT_INTERVAL;
            else if(time_interval > MAX_INTERVAL)
                time_interval = MAX_INTERVAL;
        } else {
            time_interval = MAX_INTERVAL;
        }
        log_d("Update Battery Detect Time: %d s", time_interval/1000000);
        table_index = 0;
    }

    if(mybattery.pre_voltage > 0) {
        diff = vbat - mybattery.pre_voltage;
        int idx = FIND_INDEX_BY_BAT(mybattery.pre_voltage);
        int m_stride = (capParams[idx].max - capParams[idx].min)/capParams[idx].hysteresis;
        if(diff >= m_stride) {
            adjust_voltage = ceil(m_stride * 1.0 / 2);
            time_interval = DEFAULT_INTERVAL;
        }
        else if(diff <= -(m_stride)) {
            adjust_voltage = floor(-m_stride * 1.0 / 2);
            time_interval = DEFAULT_INTERVAL;
        }
        else if(diff == 0)
            adjust_voltage = 0;
        else
            adjust_voltage = diff * ADJUST_WEIGHT;
    }
    else if(mybattery.pre_voltage == 0)
        adjust_voltage = vbat;
    log_v("Detect usb mode:%d, Real voltage: %d, pre_voltage: %d, adjust: %d",
            mybattery.battery_mode, vbat, mybattery.pre_voltage, adjust_voltage);

    vbat = mybattery.pre_voltage + adjust_voltage;
    tvTable[table_index].instVoltage = vbat;
    tvTable[table_index].sysTime = (int64_t)systick_get_time_ms();
    table_index++;
    mybattery.pre_voltage = vbat;
    return vbat;
}

static int calculate_battery_percent(int vbat)
{
    int cap, offset;
    int v0, v1, c0, c1;
    struct capacity_params* ptable = capTable.cap_params;
    if (!ptable) {
        log_e("Capacity convert tables is not present!\n");
        return -1;
    }

    if(vbat < ptable[0].min) return 0;
    if(vbat > ptable[capTable.size - 1].max) return 100;

    int index = FIND_INDEX_BY_BAT(vbat);
    cap = ptable[index].capacity;
    if ((index > 0) && (index < capTable.size -1)) {
        v0 = ptable[index].min;
        v1 = ptable[index + 1].min;
        c0 = ptable[index].capacity;
        c1 = ptable[index + 1].capacity;

        offset = ((c1 - c0) * (vbat - v0) * 10) / (v1 - v0);
        cap += offset / 10 + ((offset % 10 ) >= 5 ? 1 : 0);
        if(cap > c1) cap = c1;
        else if(cap < c0) cap = c0;
    }
    return cap;
}

static int gradient_limit_percent(int pct)
{
    int result = pct;
    if((mybattery.battery_mode == BAT_DISCHARGING && mybattery.pre_percent < pct)||
       (mybattery.battery_mode == BAT_CHARGING    && mybattery.pre_percent > pct))
        result = mybattery.pre_percent;
    if(abs(result - mybattery.pre_percent) > 1)
        result += result > mybattery.pre_percent ? 1 : -1;
    mybattery.pre_percent = result;
    return result;
}

static void voltage_detect_thread(void* data)
{
    /* step 1: init about battery all params */
    battery_params_init();
    /* step 2: init adc */
    adc_init();
    /* step 3: set timer and usb trigger battery detect */
    usb_detect_init();
    hrtimer_init(&gtimer, hrtimer_cb);
    hrtimer_start(&gtimer, DEFAULT_INTERVAL);
    /* step 4: get battery from property */
    get_battery_before_poweroff();
    /* step 5: battery voltage detect */
    while(!mStop) {
        semaphore_wait_timeout(&mybattery.detect_state, MAX_INTERVAL);
        int vbat = calculate_battery_voltage(get_battery_voltage());
        int percent = calculate_battery_percent(vbat);
        percent = gradient_limit_percent(percent);
        if(percent >= 0) {
            battery_record(percent);
            log_i("Current voltage %dmv (%d%%)\n", vbat, percent);
        }
    }
    /* step 6: deinit */
    hrtimer_cancel(&gtimer);
//    usb_detect_deinit();
    adc_deinit();
}

void battery_voltage_detect(void)
{
    thread_ptr_t thread;
    thread = thread_create("Detect battery voltage", 1024*4, (thread_func_t)voltage_detect_thread, NULL);
    thread_set_priority(thread, OS_priority_normal);
}

