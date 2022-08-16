/*
    i2c-check.c - i2c-bus driver, char device interface


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include "i2c-check.h"

struct i2c_check_dev i2c_check_data =
{
	.class = NULL,
	.nrdev = 0,
	.list = LIST_HEAD_INIT(i2c_check_data.list),
};


static struct dev_test_map test_map[] =
{
	{"name",		"test_pass", 		"test_fail"},
	{"default",		"1", 			"0"},
	{"audio_smpart_pa"      "1",			"0"},
	{"audio_hifi",		"1", 			"0"},
	{"indicator_led",	"1", 			"0"},
	{"touch_panel",		"1", 			"0"},
	{"AccelerometerSensor",	"1", 	"0"},
	{"GyroscopeSensor",	"1", 		"0"},
	{"MagnetometerSensor",	"1", 	"0"},
	{"ProximitySensor", 	"1",		"0"},
	{"LightSensor",		"1", 		"0"},

};



static ssize_t i2c_dev_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_dev_peripherals *i2c_per;
	
	list_for_each_entry(i2c_per, &i2c_check_data.list, node)
	{
		if ( !strcmp(dev_name(dev),i2c_per->name) )
		{
			break;
		}
	}
	
	ret = snprintf(buf, PAGE_SIZE, "%s\n", i2c_per->test_result);
	
	return ret;
}


static int get_test_result(struct i2c_dev_peripherals *i2c_per, char *name)
{
	int i = 0;


	for (i=0; i<ARRAY_SIZE(test_map); i++)
	{
		if ( !strcmp(name,test_map[i].name) )
		{
			if (i2c_per->status == 1)
			{
				i2c_per->test_result =  test_map[i].pass;
			}
			else
			{
				i2c_per->test_result =  test_map[i].fail;
			}

			break;
		}
	}

	// can not find in map,use default 
	if (i == ARRAY_SIZE(test_map))
	{
		if (i2c_per->status == 1)
			{
				i2c_per->test_result = "1";
			}
			else
			{
				i2c_per->test_result = "0";
			}

	}

	return 0;
	
}

static ssize_t i2c_dev_status_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct i2c_dev_peripherals *i2c_per;

	list_for_each_entry(i2c_per, &i2c_check_data.list, node)
	{
		if ( !strcmp(dev_name(dev), i2c_per->name) )
		{
			if(buf[0]=='1')
				i2c_per->status = 1;
			else
				i2c_per->status = 0;
			get_test_result(i2c_per, i2c_per->name);
			return count;
		}
	}
	
	return -1;
}

static void i2c_check_attr_init(struct device_attribute *dev_attr,
				const char *name,
				umode_t mode,
				ssize_t (*show)(struct device *dev,
						struct device_attribute *attr,
						char *buf),
				ssize_t (*store)(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t count))
{
	sysfs_attr_init(&dev_attr->attr);
	dev_attr->attr.name = name;
	dev_attr->attr.mode = mode;
	dev_attr->show = show;
	dev_attr->store = store;
}

int i2c_check_status_create(char *name,int value)
{
	int rc = 0;
	int num = 0;	
	struct i2c_dev_peripherals *i2c_per;

	if( name != NULL ){

		list_for_each_entry(i2c_per, &i2c_check_data.list, node)
		{
			if ( !strcmp(name, i2c_per->name) )
			{
				printk("%s:%s already exist\n", __func__, name);
				i2c_per->status = value;
				get_test_result(i2c_per, name);
				return 0;
			}
		}

		i2c_check_data.nrdev++;
		num = i2c_check_data.nrdev;
		

		i2c_per = kzalloc( sizeof(struct i2c_dev_peripherals), GFP_KERNEL);

		strcpy(i2c_per->name, name);
		i2c_per->index = num;
		i2c_per->status = value;
		get_test_result(i2c_per, name);

		i2c_per->dev = device_create(i2c_check_data.class, NULL, 0, NULL, name);
		if (IS_ERR(i2c_per->dev))
			pr_err("Failed to create device!\n");

		i2c_check_attr_init(&i2c_per->attr, "status", 0666, i2c_dev_status_show, i2c_dev_status_store);		

		rc = device_create_file(i2c_per->dev, &i2c_per->attr);
		if ( rc < 0)
			pr_err("Failed to create device file(%s)!\n", name);

		list_add_tail(&i2c_per->node, &i2c_check_data.list);
		  
  }
  return 0;
	
}
EXPORT_SYMBOL(i2c_check_status_create);

void i2c_check_device_register ( void )
{
	
	i2c_check_data.class = class_create(THIS_MODULE, "i2c_check");
	if (IS_ERR(i2c_check_data.class))
		pr_err("Failed to create class(i2c_check_class)!\n");
}

/*
 * module load/unload record keeping
 */
static int __init i2c_check_init(void)
{
	//int res;

	printk(KERN_INFO "i2c_check driver\n");
	i2c_check_device_register();
	return 0;
}

arch_initcall(i2c_check_init);


