#include <linux/module.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
static char buf[] ="teststr";
static char buf1[10];


void dump_stack_to_file(unsigned char *file_name)
{
	struct pt_regs regs;

	
    unsigned long sp;
	unsigned long ra;
	unsigned long pc;

    char *modname;
	const char *name;
	unsigned long offset, size;
	char namebuf[KSYM_NAME_LEN+1];
    char buffer[sizeof("%s+%#lx/%#lx [%s]") + KSYM_NAME_LEN +
		    2*(BITS_PER_LONG*3/10) + MODULE_NAME_LEN + 1];

   

    //
    unsigned long address;

    
    struct file *fp;
    mm_segment_t fs;
    loff_t pos;

    char *file_buf = NULL;
    char *file_buf_pos = file_buf;
    int buf_len = 0;
    int i_ret;

    file_buf = kmalloc(5000, GFP_ATOMIC);
    if (file_buf == NULL) {
        printk("kmalloc  error\n");
        return ;
    }
    memset(file_buf, 0, 5000);

    prepare_frametrace(&regs);

    sp = regs.regs[29];
	ra = regs.regs[31];
	pc = regs.cp0_epc;

    i_ret = sprintf(file_buf_pos, "Call Trace:\n");
    file_buf_pos += i_ret;
	printk("dump_stack_to_file Call Trace:\n");
	do {
        i_ret = sprintf(file_buf_pos, "[<%08lx>]", pc);
        file_buf_pos += i_ret;
	    printk("[<%08lx>]", pc);

        address = (unsigned long)__builtin_extract_return_addr((void *)pc);
        name = kallsyms_lookup(address, &size, &offset, &modname, namebuf);

    	if (!name)
    		sprintf(buffer, "0x%lx", address);
    	else {
    		if (modname)
    			sprintf(buffer, "%s+%#lx/%#lx [%s]", name, offset,
    				size, modname);
    		else
    			sprintf(buffer, "%s+%#lx/%#lx", name, offset, size);
    	}

        i_ret = sprintf(file_buf_pos, " %s\n", buffer);
        file_buf_pos += i_ret;
        printk(" %s\n", buffer);
        
		pc = unwind_stack(current, &sp, pc, &ra);
	} while (pc);
    i_ret = sprintf(file_buf_pos, "\n");
    file_buf_pos += i_ret;
    buf_len = file_buf_pos - file_buf;
	printk("\n");

    printk("file_buf:%s \n", file_buf);
    printk("file_buf_len:%d \n", buf_len);

    /* write to file */    
    fp =filp_open(file_name, O_RDWR | O_CREAT, 0644);
    
    if (IS_ERR(fp)){ 
        printk("create file error\n");
        return ;
    } 

    
    fs =get_fs();
    set_fs(KERNEL_DS);
    pos =0; 
    i_ret = vfs_write(fp, file_buf, buf_len, &pos);
    printk("vfs_write size:%d \n", i_ret);
    pos =0; 

    filp_close(fp, NULL);
    set_fs(fs);

    return ;
}
  
int __init hello_init(void)
{ 
	printk("hello init\n");
#if 1
    struct file *fp;
    mm_segment_t fs;
    loff_t pos;
	
    fp =filp_open("/home/develop/ctest/kernel/writefile.test",O_RDWR | O_CREAT,0644);
    if (IS_ERR(fp)){ 
        printk("create file error\n");
        return -1;
    } 
    fs =get_fs();
    set_fs(KERNEL_DS);
    pos =0; 
    vfs_write(fp,buf, sizeof(buf), &pos);
    pos =0; 
    vfs_read(fp,buf1, sizeof(buf), &pos);
    printk("read: %s\n",buf1);
    filp_close(fp,NULL);
    set_fs(fs);
#endif	
    return 0;
} 
void __exit hello_exit(void)
{ 
    printk("hello exit\n");
} 
  
module_init(hello_init);
module_exit(hello_exit);
  
MODULE_LICENSE("GPL");