///
/// \file CardFactory.cxx
/// \author Pascal Boeschoten
///

#include "RORC/CardFactory.h"
#include <dirent.h>
#include "RORC/CardDummy.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#include <pda.h>
#include "CardPdaCrorc.h"
#endif
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

CardFactory::CardFactory()
{
}

CardFactory::~CardFactory()
{
}

std::shared_ptr<CardInterface> CardFactory::getCardFromSerialNumber(int serialNumber)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == DUMMY_SERIAL_NUMBER) {
    return std::make_shared<CardDummy>();
  }

  // TODO this could use some cleanup and Cplusplusification

  std::shared_ptr<CardInterface> cardSharedPtr;
  std::vector<char*> ids = { "0000 0000", "\0" };
  DIR *directory = NULL;         // Pointer to the directory that lists the PCI devices.
  struct dirent *file;           // For iterating through the PCI devices.
  FILE *vendorFp, *deviceFp;   // File pointer to the 'vendor' and 'device' files.
  char device[51],               // String of the 'device' file path.
      vendor[51],               // String of the 'vendor' file path.
      deviceId[6],             // String of the device ID (found in the 'device' file).
      vendorId[6];             // String of the vendor ID (found in the 'vendor' file).
  //fullId[10];              // Vendor ID and device ID together.
  //char cruId[] = "e001";        // Device ID of the CRU cards.
  char crorcId[] = "0033";      // Device ID of the CRORC cards.
  char cernId[] = "0x10dc";     // Vendor ID of CERN devices.

  // Opening the folder of the PCI devices.
  directory = opendir("/sys/bus/pci/devices/");
  if (directory == NULL) {
    printf("Can't open directory\n");
  }

  // Iterating through the devices to find the cards.
  while (NULL != (file = readdir(directory))) {

    // Opening the 'vendor' file.
    snprintf(vendor, sizeof(vendor), "%s%s/vendor", "/sys/bus/pci/devices/", file->d_name);
    vendorFp = fopen(vendor, "r");
    if (vendorFp == NULL) {
      continue;
    }

    // Getting the vendor ID from the 'vendor' file.
    fscanf(vendorFp, "%s", vendorId);

    // Comparing the vendor ID with the CERN vendor ID
    if (strcmp(vendorId, cernId) == 0) {

      // Opening the 'device' file.
      snprintf(device, sizeof(device), "%s%s/device", "/sys/bus/pci/devices/", file->d_name);
      //printf("%s\n", device);
      deviceFp = fopen(device, "r");
      if (deviceFp == NULL) {
        ALICEO2_RORC_THROW_EXCEPTION("Failed to open device file");
      }

      // Getting the device ID from the 'device' file.
      fscanf(deviceFp, "%s", deviceId);
      printf("Device ID: %s\n", deviceId);

      // Cutting the starting '0x' off from the string of the IDs.
      memmove(deviceId, deviceId + 2, strlen(deviceId) - 1);
      memmove(vendorId, vendorId + 2, strlen(vendorId) - 1);

      // Opening the 'config' file to get the revision ID.
      snprintf(device, sizeof(device), "%s%s/config", "/sys/bus/pci/devices/", file->d_name);
      FILE *cfp = fopen(device, "rb");
      char cfg[64];

      fread(&cfg, 1, sizeof(cfg), cfp);

      // If device ID matches with the CRORC ID it's a CRORC.
      if (strcmp(deviceId, crorcId) == 0) {
        //The card is a CRORC
        printf("The card is a CRORC\n");
        ids[0] = "10dc 0033";
        ids[1] = NULL;

        // Checking if the PDA module (uio_pci_dma.ko) is inserted.
        if(PDAInit() != PDA_SUCCESS){
          ALICEO2_RORC_THROW_EXCEPTION("Failed to initialize PDA driver, is kernel module inserted?");
        }

        // Get device operator
        DeviceOperator* deviceOperator = DeviceOperator_new((const char**)ids.data(), PDA_ENUMERATE_DEVICES );
        if(deviceOperator == NULL){
          ALICEO2_RORC_THROW_EXCEPTION("Unable to get device operator");
        }

        // Getting the card.
        PciDevice* pciDevice;
        if(PDA_SUCCESS != DeviceOperator_getPciDevice(deviceOperator, &pciDevice, serialNumber) ){
          if(PDA_SUCCESS != DeviceOperator_delete(deviceOperator, PDA_DELETE) ){
            ALICEO2_RORC_THROW_EXCEPTION("Failed to get PCI Device; Cleanup failed");
          }
          ALICEO2_RORC_THROW_EXCEPTION("Failed to get PCI Device");
        }

        if (!cardSharedPtr) {
          cardSharedPtr = std::make_shared<CardPdaCrorc>(deviceOperator, pciDevice, serialNumber);
        }

      } else {
        //The card is a CRU

        ALICEO2_RORC_THROW_EXCEPTION("CRU not yet supported");

        printf("The card is a CRU\n");
        ids[0] = "10dc e001";
        ids[1] = NULL;
      }
      fclose(deviceFp);
      fclose(vendorFp);
      break;
    }
    fclose(vendorFp);

  }  //Only one card in one machine?

  closedir(directory);

  return cardSharedPtr;
#else
#pragma message("PDA not enabled, Alice02::Rorc::CardFactory::getCardFromSerialNumber() will always return dummy implementation")
  return std::make_shared<CardDummy>();
#endif
}

} // namespace Rorc
} // namespace AliceO2
