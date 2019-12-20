#include <linux/module.h>

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/uaccess.h>

static int lacc_major = 300;
static char *lacc_mem = NULL;
static int lacc_mem_size = 0x400000;

struct ioctl_data {
	uint16_t map_h;
	uint16_t map_w;
	uint16_t times;
	uint16_t nums;
	uint32_t points;
	uint32_t result;
};

static int lacc_open(struct inode *inode, struct file *file) {
	printk("lacc: open\n");
	return 0;
}

static int lacc_close(struct inode *inode, struct file *file)
{
	printk("lacc: close\n");
	return 0;
}

static long lacc_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	struct ioctl_data idata;

	if (copy_from_user(&idata, (void *)arg, sizeof(idata))) {
		printk("lacc: copy failed\n");
		return -EFAULT;
	}
		
	printk("lacc: ioctl, cmd=%d\n", cmd);
	idata.result = 0x12345678;

	if (copy_to_user((void *)arg, &idata, sizeof(idata))) {
		return -EFAULT;
	}
	return 111;
}

static int lacc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = PAGE_ALIGN(vma->vm_end - vma->vm_start);

	if (size > lacc_mem_size || !lacc_mem) {
		printk("lacc: invalid\n");
		return -EINVAL;
	}
	return remap_pfn_range(vma, start, (virt_to_phys(lacc_mem) >> PAGE_SHIFT), size, PAGE_SHARED);
}

static int lacc_release(struct inode *inode, struct file *file)
{
	struct page *page;
	for (page = virt_to_page(lacc_mem); page < virt_to_page(lacc_mem + lacc_mem_size); page++) {
		ClearPageReserved(page);
	}
	return 0;
}

static const struct file_operations lacc_fops = {
	.owner		= THIS_MODULE,
	.open		= lacc_open,
	.unlocked_ioctl	= lacc_ioctl,
	.mmap           = lacc_mmap,
	.release        = lacc_release,
};

static int __init lacc_init(void)
{
	int err = -EIO;
	printk("lacc: init, major=%d\n", lacc_major);
	lacc_mem = kmalloc(lacc_mem_size, GFP_KERNEL);
	if (!lacc_mem)
		return -ENOMEM;
	
	if (register_chrdev(lacc_major, "lacc", &lacc_fops)) {
		printk("lacc: register failed\n");
		kfree(lacc_mem);
		return err;
	}
	return 0;
}
subsys_initcall(lacc_init);
