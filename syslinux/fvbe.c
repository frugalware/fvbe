#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/pci.h>

#define error(S) fprintf(stdout,"%s: %s\n",__func__,S)

static bool pci_bus_probe(void)
{
  struct pci_device *device = 0;
  struct pci_domain *domain = 0;
  uint16_t vendor = 0;
  uint16_t product = 0;
  uint16_t sub_vendor = 0;
  uint16_t sub_product = 0;
  uint8_t revision = 0;
  uint32_t class = 0;

  if((domain = pci_scan()) == 0)
  {
    error("failed to probe pci bus");
    return false;
  }

  for_each_pci_func(device,domain)
  {
    vendor = device->vendor;

    product = device->product;

    sub_vendor = device->sub_vendor;

    sub_product = device->sub_product;

    revision = device->revision;

    class = (device->class[2] << 16) | (device->class[1] << 8) | (device->class[0] << 0);
  }

  free_pci_domain(domain);

  return true;
}

extern int main(void)
{
  if(!pci_bus_probe())
    return EXIT_FAILURE;

  sleep(10);

  return EXIT_SUCCESS;
}
