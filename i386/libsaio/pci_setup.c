#include "libsaio.h"
#include "bootstruct.h"
#include "pci.h"

extern void set_eth_builtin(pci_dt_t *eth_dev);
extern bool setup_nvidia_devprop(pci_dt_t *nvda_dev);
extern bool setup_ati_devprop(pci_dt_t *ati_dev);
extern int ehci_acquire(pci_dt_t *pci_dev);
extern int legacy_off(pci_dt_t *pci_dev);
extern int uhci_reset(pci_dt_t *pci_dev);
extern void force_enable_hpet(pci_dt_t *lpc_dev);

void setup_pci_devs(pci_dt_t *pci_dt)
{
	char *devicepath;
	BOOL do_eth_devprop, do_gfx_devprop, fix_ehci, fix_legoff, fix_uhci, fix_usb, do_enable_hpet;
	pci_dt_t *current = pci_dt;

	do_eth_devprop = do_gfx_devprop = fix_ehci = fix_legoff = fix_uhci = fix_usb = do_enable_hpet = false;

	getBoolForKey("EthernetBuiltIn", &do_eth_devprop, &bootInfo->bootConfig);
	getBoolForKey("GraphicsEnabler", &do_gfx_devprop, &bootInfo->bootConfig);
	if (getBoolForKey("USBBusFix", &fix_usb, &bootInfo->bootConfig) && fix_usb)
		fix_ehci = fix_uhci = true;
	else
	{
		getBoolForKey("EHCIacquire", &fix_ehci, &bootInfo->bootConfig);
		getBoolForKey("UHCIreset", &fix_uhci, &bootInfo->bootConfig);
	}
	getBoolForKey("USBLegacyOff", &fix_legoff, &bootInfo->bootConfig);
	getBoolForKey("ForceHPET", &do_enable_hpet, &bootInfo->bootConfig);

	while (current)
	{
		devicepath = get_pci_dev_path(current);

		switch (current->class_id)
		{
			case PCI_CLASS_NETWORK_ETHERNET: 
				if (do_eth_devprop)
					set_eth_builtin(current);
				break;
				
			case PCI_CLASS_DISPLAY_VGA:
				if (do_gfx_devprop)
					switch (current->vendor_id)
					{
						case PCI_VENDOR_ID_ATI:
							verbose("ATI VGA Controller [%04x:%04x] :: %s \n", 
							current->vendor_id, current->device_id, devicepath);
							setup_ati_devprop(current); 
							break;
					
						case PCI_VENDOR_ID_INTEL: 
							/* message to be removed once support for these cards is added */
							verbose("Intel VGA Controller [%04x:%04x] :: %s (currently NOT SUPPORTED)\n", 
								current->vendor_id, current->device_id, devicepath);
							break;
					
						case PCI_VENDOR_ID_NVIDIA: 
							setup_nvidia_devprop(current);
							break;
					}
				break;

			case PCI_CLASS_SERIAL_USB:
				switch (pci_config_read8(current->dev.addr, PCI_CLASS_PROG))
				{
					/* EHCI */
					case 0x20:
				    	if (fix_ehci)
							ehci_acquire(current);
						if (fix_legoff)
							legacy_off(current);
						break;

					/* UHCI */
					case 0x00:
				    	if (fix_uhci)
							uhci_reset(current);
						break;
				}
				break;

			case PCI_CLASS_BRIDGE_ISA:
				if (do_enable_hpet)
					force_enable_hpet(current);
				break;
		}
		
		setup_pci_devs(current->children);
		current = current->next;
	}
}
