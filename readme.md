这是一个用于将chromebook的键盘布局改为普通PC的键盘布局的命令行工具。

硬件提供的是扫描码，而软件使用的键盘码。扫描码和键盘码之间的映射关系是由内核驱动完成的。而在Linux内核中，有一个input子系统，通过它，我们可以改变键盘映射的规则。在input子系统的ioctl中就提供了相关的功能。