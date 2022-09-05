#include <errno.h>
#include <stdio.h>
#include <usbg/usbg.h>

int _remove_gadget(usbg_gadget *g)
{
	int usbg_ret;
	usbg_udc *u;

	/* Check if gadget is enabled */
	u = usbg_get_gadget_udc(g);

	/* If gadget is enable we have to disable it first */
	if (u) {
		usbg_ret = usbg_disable_gadget(g);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error on USB disable gadget udc\n");
			fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
			goto out;
		}
	}

	/* Remove gadget with USBG_RM_RECURSE flag to remove
	 * also its configurations, functions and strings */
	usbg_ret = usbg_rm_gadget(g, USBG_RM_RECURSE);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget remove\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
	}

out:
	return usbg_ret;
}

int remove_gadget(uint16_t device_vid, uint16_t device_pid)
{
	int usbg_ret;
	int ret = -EINVAL;
	usbg_state *s;
	usbg_gadget *g;
	struct usbg_gadget_attrs g_attrs;

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB state init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	g = usbg_get_first_gadget(s);
	while (g != NULL) {
		/* Get current gadget attrs to be compared */
		usbg_ret = usbg_get_gadget_attrs(g, &g_attrs);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error on USB get gadget attrs\n");
			fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
			goto out2;
		}

		/* Compare attrs with given values and remove if suitable */
		if (g_attrs.idVendor == device_vid && g_attrs.idProduct == device_pid) {
			usbg_gadget *g_next = usbg_get_next_gadget(g);

			usbg_ret = _remove_gadget(g);
			if (usbg_ret != USBG_SUCCESS)
				goto out2;

			g = g_next;
		} else {
			g = usbg_get_next_gadget(g);
		}
	}

out2:
	usbg_cleanup(s);
out1:
    printf("Device %04x:%04x removed", device_vid, device_pid);
	return ret;
}
