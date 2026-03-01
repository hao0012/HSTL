# thread pool
## conditional variable block
https://www.zhihu.com/question/587575043

`closed_` must be modified with lock holding, otherwise the cv may block infinitly.


## lock contention

one queue:
```
thread_count: 1, task_count: 1000000, average cost time: 1.86159
thread_count: 2, task_count: 1000000, average cost time: 1.20177
thread_count: 3, task_count: 1000000, average cost time: 1.90434
thread_count: 4, task_count: 1000000, average cost time: 2.73606
thread_count: 8, task_count: 1000000, average cost time: 4.57585
thread_count: 16, task_count: 1000000, average cost time: 4.07475
thread_count: 32, task_count: 1000000, average cost time: 3.839
thread_count: 64, task_count: 1000000, average cost time: 3.72768
thread_count: 128, task_count: 1000000, average cost time: 3.6895
```

multi queue:
```
thread_count: 1, task_count: 1000000, average cost time: 1.3102
thread_count: 2, task_count: 1000000, average cost time: 0.720193
thread_count: 3, task_count: 1000000, average cost time: 0.973741
thread_count: 4, task_count: 1000000, average cost time: 1.29421
thread_count: 8, task_count: 1000000, average cost time: 2.27094
thread_count: 16, task_count: 1000000, average cost time: 2.26008
thread_count: 32, task_count: 1000000, average cost time: 2.27009
thread_count: 64, task_count: 1000000, average cost time: 2.50416
thread_count: 128, task_count: 1000000, average cost time: 3.16367
```