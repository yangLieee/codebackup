#!/bin/bash

cp241 (){
  scp $1 user@10.1.4.241:~/$2
}

cpf241 (){
  scp -r $1 user@10.1.4.241:~/$2
}

cpall (){
  scp -r * user@10.1.4.241:~/$1
}



cpopar()
{
  scp -r $1  user@10.1.4.241:/home/user/work/remote/$3
  if [ "$3"x == ""x ]; then
    remote_dir=$1;
  else
    remote_dir=$3
  fi
  if [ "$2"x == ""x ]; then
    ssh -p 22 user@10.1.4.241 "cd /home/user/work/remote ; adb push ${remote_dir} /storage/"
  else
    ssh -p 22 user@10.1.4.241 "cd /home/user/work/remote ; adb push ${remote_dir} $2"
  fi
}


#tmux
alias tmuxne="tmux new -s"
alias tmuxre="tmux attach-session -t"
alias tmuxrm="tmux kill-session -t"
