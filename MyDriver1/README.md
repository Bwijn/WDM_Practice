### Hide and Protect  process 

主要就是

1. obregistercallback降权

2. 改pid隐藏process 从这个大神这里借鉴的：

   [[系统底层\] win10-win11进程隐藏小技巧]: https://www.52pojie.cn/forum.php?mod=viewthread&amp;tid=1866668&amp;extra=&amp;highlight=%BD%F8%B3%CC%D2%FE%B2%D8&amp;page=1

   

todo

- [ ]  it cannot clear the residual memory

  因为pid为4系统不会回收 eprocess的资源，pid restore也不行

  

