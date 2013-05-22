#! /bin/bash

# local 
cd /home/qinyuchun/source/msp-c/script/

# femoo
#cd /data/HA5/msp-c/script/

# fetion
#cd /home/fetion/svr/msp-c/script/

tar zcvf mspc`date -d last-day +%Y%m%d`.tar.gz  mspc`date -d last-day +%Y%m%d_*`.log
rm -f mspc`date -d last-day +%Y%m%d_*`.log
rm -f mspc`date -d '7 days ago' +%Y%m%d`.tar.gz

tar zcvf route`date -d last-day +%Y%m%d`.tar.gz  route`date -d last-day +%Y%m%d_*`.log
rm -f route`date -d last-day +%Y%m%d_*`.log
rm -f route`date -d '7 days ago' +%Y%m%d`.tar.gz
