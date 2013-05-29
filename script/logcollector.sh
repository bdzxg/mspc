#! /bin/bash

cd /data/mspclog1

tar zcvf mspc`date -d last-day +%Y%m%d`.tar.gz  mspc*`date -d last-day +%Y%m%d_*`.log
rm -f mspc*`date -d last-day +%Y%m%d_*`.log
rm -f mspc`date -d '7 days ago' +%Y%m%d`.tar.gz

tar zcvf route`date -d last-day +%Y%m%d`.tar.gz  route*`date -d last-day +%Y%m%d_*`.log
rm -f route*`date -d last-day +%Y%m%d_*`.log
rm -f route`date -d '7 days ago' +%Y%m%d`.tar.gz


cd /data/mspclog2

tar zcvf mspc`date -d last-day +%Y%m%d`.tar.gz  mspc*`date -d last-day +%Y%m%d_*`.log
rm -f mspc*`date -d last-day +%Y%m%d_*`.log
rm -f mspc`date -d '7 days ago' +%Y%m%d`.tar.gz

tar zcvf route`date -d last-day +%Y%m%d`.tar.gz  route*`date -d last-day +%Y%m%d_*`.log
rm -f route*`date -d last-day +%Y%m%d_*`.log
rm -f route`date -d '7 days ago' +%Y%m%d`.tar.gz
