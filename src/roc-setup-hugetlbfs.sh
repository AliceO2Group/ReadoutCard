#!/bin/bash

# Script for setting up hugetlbfs pages & mounts.
# By default allocates 128 of 2 MiB hugepages and 0 of 1 GiB hugepages.
# Defaults can be altered by configuration files containing an integer (see variables HUGEPAGES_XX_CONF for paths).


HUGEPAGES_2M_CONF="/etc/flpprototype.d/hugepages-2MiB.conf"
HUGEPAGES_2M_SYSFILE="/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"
HUGEPAGES_2M_NUMBER=128
HUGEPAGES_1G_CONF="/etc/flpprototype.d/hugepages-1GiB.conf"
HUGEPAGES_1G_SYSFILE="/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages"
HUGEPAGES_1G_NUMBER=0


# Allocate hugepages of each type
if [ -f $HUGEPAGES_2M_CONF ]; then
  HUGEPAGES_2M_NUMBER=$(cat $HUGEPAGES_2M_CONF)
  echo "Allocating configured amount of 2 MiB hugepages: ${HUGEPAGES_2M_NUMBER}"
else
  echo "Allocating default amount of 2 MiB hugepages: ${HUGEPAGES_2M_NUMBER}"
fi
echo $HUGEPAGES_2M_NUMBER > $HUGEPAGES_2M_SYSFILE

if [ -f $HUGEPAGES_1G_CONF ]; then
  HUGEPAGES_2M_NUMBER=$(cat $HUGEPAGES_1G_CONF)
  echo "Allocating configured amount of 1 GiB hugepages: ${HUGEPAGES_1G_NUMBER}"
else
  echo "Allocating default amount of 1 GiB hugepages: ${HUGEPAGES_1G_NUMBER}"
fi
echo $HUGEPAGES_1G_NUMBER > $HUGEPAGES_1G_SYSFILE


# Create hugetlbfs mounts in /var/lib/hugetlbfs/global/...
echo ""
echo "Creating hugetlbfs mounts"
hugeadm --create-global-mounts


# Display hugepage status
echo ""
echo "Hugepages:"
hugeadm --pool-list
echo ""
echo "Note: use 'echo [number] > /sys/kernel/mm/hugepages/hugepages-[size]/nr_hugepages' to allocate more hugepages"