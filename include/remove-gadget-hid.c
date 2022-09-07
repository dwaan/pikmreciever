#include <errno.h>
#include <stdio.h>
#include <usbg/usbg.h>

int _remove_gadget(usbg_gadget *gadget)
{
	int usbg_ret;
	usbg_udc *controller;

	/* Check if gadget is enabled */
	controller = usbg_get_gadget_udc(gadget);

	/* If gadget is enable we have to disable it first */
	if (controller)
	{
		usbg_ret = usbg_disable_gadget(gadget);
		if (usbg_ret != USBG_SUCCESS)
		{
			fprintf(stderr, "Error on USB disable gadget udc\n");
			fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
			goto out;
		}
	}

	/* Remove gadget with USBG_RM_RECURSE flag to remove
	 * also its configurations, functions and strings */
	usbg_ret = usbg_rm_gadget(gadget, USBG_RM_RECURSE);
	if (usbg_ret != USBG_SUCCESS)
	{
		fprintf(stderr, "Error on USB gadget remove\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
	}

out:
	return usbg_ret;
}

int remove_gadget(uint16_t *device_vid, uint16_t *device_pid)
{
	int usbg_ret;
	int ret = -EINVAL;
	usbg_state *state;
	usbg_gadget *gadget;
	struct usbg_gadget_attrs g_attrs;

	usbg_ret = usbg_init("/sys/kernel/config", &state);
	if (usbg_ret != USBG_SUCCESS)
	{
		fprintf(stderr, "Error on USB state init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	gadget = usbg_get_first_gadget(state);
	while (gadget != NULL)
	{
		/* Get current gadget attrs to be compared */
		usbg_ret = usbg_get_gadget_attrs(gadget, &g_attrs);
		if (usbg_ret != USBG_SUCCESS)
		{
			fprintf(stderr, "Error on USB get gadget attrs\n");
			fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
			goto out2;
		}

		/* Compare attrs with given values and remove if suitable */
		if (g_attrs.idVendor == *device_vid && g_attrs.idProduct == *device_pid)
		{
			usbg_gadget *g_next = usbg_get_next_gadget(gadget);

			usbg_ret = _remove_gadget(gadget);
			if (usbg_ret != USBG_SUCCESS)
				goto out2;

			gadget = g_next;
		}
		else
		{
			gadget = usbg_get_next_gadget(gadget);
		}
	}

out2:
	usbg_cleanup(state);
out1:
	printf("Device with vid: 0x%04x and pid: 0x%04x removed\n", *device_vid, *device_pid);
	return ret;
}
