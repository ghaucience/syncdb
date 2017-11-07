#!/bin/bash

export LD_LIBRARY_PATH="/usr/local/lib/sac"

base=`pwd`
app="dbsync_cli"

is_client_run() {
        cnt=`pidof dbsync_cli | wc -l`
        if [ "$cnt" == "0" ]; then
                return "0"
        fi
        return "1"
}


do_start_demo() {
        while [ 1 ]; do
                ${base}/${app} > /dev/null
                sleep 1
        done
}

do_start() {
        is_client_run
        ret=$?
        if [ "$ret" != "0" ]; then
                echo "client has startted!"
                return
        fi

        do_start_demo &
}

do_stop() {
        is_client_run
        ret=$?
        if [ "$ret" == "0" ]; then
                echo "client does't start!"
                return
        fi

        echo "stop client"

        pid=`ps aux | grep client.sh | grep start | xargs | cut -d " " -f 2`
        kill -9 $pid
        kill -9 `pidof dbsync_cli`
}



case $1 in
"start")
        do_start
        ;;
"stop")
        do_stop
        ;;
*)
        echo "not supportted cmd"
        ;;
esac
