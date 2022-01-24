#include <cstdio>
#include <string>

#include <verbs.h>
#include <arch.h>

int main()
{
	int num_devices;

    struct ibv_device** device_list = ibv_get_device_list(&num_devices);
    if (!device_list) {
        fprintf(stderr, "Unable to get HCA device list\n");
        exit(1);
    }

    if (num_devices == 0) {
        fprintf(stderr, "Unable to find any HCA devices\n");
        ibv_free_device_list(device_list);
        exit(1);
    }

    for (int i = 0; i < num_devices; i++) {
        fprintf(stdout, "Avialable HCA device %s\n", ibv_get_device_name(device_list[i]));
    }
    ibv_free_device_list(device_list);

    return 0;
}