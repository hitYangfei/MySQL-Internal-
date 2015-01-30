# ==== Purpose ====
#
# Watis until DBSCALE SHOW DATASOURCE soure_name , colunm status
# is Working.
#
# ==== Usage ====
# 
# [--let $data_source_name= source_name]
# [--let $status= Status]
# --source dbscale_include/wait_for_data_source_working.inc
#
# Parameters:
#
# $data_source_name
#   The name of data source whick need to check working status.
#   MusT initialized it before use this inc
#
# $status
#   The status column name of the output of DBSCLE SHOW DATASOURCE ..
#   Beceuse the different type datasource the status column name
#   is different. eg:
#           Server       SourceStatus
#           LoadBalance  Status
#



--let $_wait_continue= 1

if ($data_source_name)
{
  --let $wait_sql= `SELECT concat("DBSCALE SHOW DATASOURCE ","$data_source_name")`
  while($_wait_continue)
  {
    --let $_result= query_get_value($wait_sql, $status, 1)
    if ($_result == 'Working')
    {
      --echo [waiting source $data_source_name is working]
      --let $_wait_continue= 0
    }
    sleep 1;
  }
}
