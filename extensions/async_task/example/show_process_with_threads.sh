ps -T -ef | egrep -v "(grep|vim)" | grep async_task_example
