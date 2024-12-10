/**
 * Keyboard Platform Device Driver
 * 
 * Noah Netz
*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kstrtox.h>



#define KB_BUFFER_OFFSET 0x0
#define BYTE_SIZE 16




/**
 * Define the compatible property used for matching devices to this driver,
 * then add out device id structure to the kernel's device table. For a device
 * to be matched with this driver, its device tree node must use the same
 * compatible string as defined here.
 */
static const struct of_device_id keyboard_of_match[] =
{
	{.compatible = "dupuis,keyboard",},
	{}
};

/**
 * struct keyboard_dev - Private keyboard device struct.
 * @base_addr:        Pointer to the component's base address
 * @red_duty_cycle:   Address of the kb_buffer register
 * @lock:             mutex used to prevent concurrent writes to memory
 *
 * keyboard_dev struct gets created for each keyboard component.
 */
struct keyboard_dev
{
	void __iomem *base_addr;
	void __iomem *kb_buffer;
	struct mutex lock;
};



// ATTRIBUTES -----------------------------------------------------------------

/**
 * kb_buffer_show() - Return the kb_buffer value to userspace via sysfs.
 * @dev:  Device structure for the keyboard component. This is embedded
 *        in the keyboard's platform device struct.
 * @attr: Unused.
 * @buf:  Buffer that gets returned to userspace.
 * 
 * Return: The number of bytes read.
 */
static ssize_t kb_buffer_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned int kb_buffer;
	struct keyboard_dev *priv = dev_get_drvdata(dev);
	
	kb_buffer = ioread32(priv->kb_buffer);
	
	return scnprintf(buf, PAGE_SIZE, "%u\n", kb_buffer);
}

/**
 * kb_buffer_store() - Attempt to store the kb_buffer value.
 */ 
static ssize_t kb_buffer_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_err("Keyboard buffer cannot be written to.");
	return size;
}



// Define sysfs attributes
static DEVICE_ATTR_RW(kb_buffer);

// Create an attribute group so the device core can export attributes for us
static struct attribute *keyboard_attrs[] =
{
	&dev_attr_kb_buffer.attr,
	NULL,
};
ATTRIBUTE_GROUPS(keyboard);

// END OF ATTRIBUTES ----------------------------------------------------------



// PROBE AND REMOVE -----------------------------------------------------------

/**
 * keyboard_probe() - Initialize keyboard device when a match is found.
 * @pdev: Platform device structure associated with keyboard device;
 *        pdev is automatically created by the driver core based upon
 *        the device tree node.
 *
 * It's called by the kernel when a keyboard device is found in the device tree.
 */
static int keyboard_probe(struct platform_device *pdev)
{
	pr_info("keyboard_probe\n");
	
	/**
	 * Allocate kernel memory for the pwm device and set it to 0.
	 * GFP_KERNEL specifies that we are allocating normal kernel RAM;
	 * see the kmalloc documentation for more info. The allocated memory
	 * is automatically freed when the device is removed.
	 */
	struct keyboard_dev *priv;
	priv = devm_kzalloc(&pdev->dev, sizeof(struct keyboard_dev),
		GFP_KERNEL);
	if (!priv)
	{
		pr_err("Failed to allocate memory.\n");
		return -ENOMEM;
	}
	
	/**
	 * Request and remap the device's memory region. Requesting the region
	 * make sure nobody else can use that memory. The memory is remapped
	 * into the kernel's virtual address space because we don't have access
	 * to physical memory locations.
	 */
	priv->base_addr = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base_addr))
	{
		pr_err("Failed to request/remap platform device resource.\n");
		return PTR_ERR(priv->base_addr);
	}
	
	// Set the memory addresses for each register.
	priv->kb_buffer = priv->base_addr + KB_BUFFER_OFFSET;
	
	/**
	 * Attach the pwm's private data to the platform device's struct.
	 * This is so we can access our state container in the other functions.
	 */
	platform_set_drvdata(pdev, priv);
	
	pr_info("keyboard_probe successful! :)\n");
	return 0;
}



/**
 * keyboard_remove() - Remove a keyboard device.
 * @pdev: Platform device structure associated with our keyboard device.
 * 
 * It's called when an keyboard device is removed or the driver is removed.
 */
static int keyboard_remove(struct platform_device *pdev)
{	
	pr_info("keyboard_remove successful! :)\n");
	return 0;
}

// END OF PROBE AND REMOVE ----------------------------------------------------



/**
 * struct keyboard_driver - Platform driver struct for this driver
 * @probe:                 Pointer to function called when device is found
 * @remove:                Pointer to function called when device is removed
 * @driver.owner:          Which module owns this driver
 * @driver.name:           Name of driver
 * @driver.of_match_table: Device tree match table
 */
static struct platform_driver keyboard_driver = {
	.probe = keyboard_probe,
	.remove = keyboard_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "keyboard",
		.of_match_table = keyboard_of_match,
		.dev_groups = keyboard_groups,
	},
};



module_platform_driver(keyboard_driver);

MODULE_DEVICE_TABLE(of, keyboard_of_match);
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Noah Netz");
MODULE_DESCRIPTION("keyboard driver");
