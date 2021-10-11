ORACLE_HOME=/u01/app/oracle/products/10.2.0/db

PATH=/usr/bin:/etc:/usr/sbin:/usr/ucb:$HOME/bin:/usr/bin/X11:/sbin:.

export PATH
export ORACLE_BASE=/u01/app/oracle
export TNS_ADMIN=$ORACLE_HOME/network/admin
export ORACLE_HOME=$ORACLE_BASE/products/10.2.0/db
export LD_LIBRARY_PATH=$ORACLE_HOME/lib:$ORACLE_HOME/lib32:
export LIBPATH=$LD_LIBRARY_PATH
export PATH=$ORACLE_HOME/bin:$PATH
export ORACLE_SID=orcl

./chuck
