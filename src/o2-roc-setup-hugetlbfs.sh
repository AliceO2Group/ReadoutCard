#!/bin/bash

# Script for setting up hugetlbfs pages & mounts.
# By default allocates 128 of 2 MiB hugepages and 6 of 1 GiB hugepages.
# Defaults can be altered by configuration files containing an integer (see variables HUGEPAGES_XX_CONF for paths).


HUGEPAGES_2M_CONF=/etc/flp.d/readoutcard/hugepages-2MiB.conf
HUGEPAGES_2M_SYSFILE=/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
HUGEPAGES_2M_NUMBER=128
HUGEPAGES_1G_CONF=/etc/flp.d/readoutcard/hugepages-1GiB.conf
HUGEPAGES_1G_SYSFILE=/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages
HUGEPAGES_1G_NUMBER=6


# Allocate hugepages of each type
echo -n "File '${HUGEPAGES_2M_CONF}' "
if [ -f $HUGEPAGES_2M_CONF ]; then
  HUGEPAGES_2M_NUMBER=$(cat $HUGEPAGES_2M_CONF)
  echo "found, allocating configured amount of 2 MiB hugepages: ${HUGEPAGES_2M_NUMBER}"
else
  echo "not found, allocating default amount of 2 MiB hugepages: ${HUGEPAGES_2M_NUMBER}"
fi
echo $HUGEPAGES_2M_NUMBER > $HUGEPAGES_2M_SYSFILE

echo -n "File '${HUGEPAGES_1G_CONF}' "
if [ -f $HUGEPAGES_1G_CONF ]; then
  HUGEPAGES_1G_NUMBER=$(cat $HUGEPAGES_1G_CONF)
  echo "found, allocating configured amount of 1 GiB hugepages: ${HUGEPAGES_1G_NUMBER}"
else
  echo "not found, allocating default amount of 1 GiB hugepages: ${HUGEPAGES_1G_NUMBER}"
fi
echo $HUGEPAGES_1G_NUMBER > $HUGEPAGES_1G_SYSFILE

# Create hugetlbfs mounts in /var/lib/hugetlbfs/global/...
echo "Creating hugetlbfs mounts"
hugeadm --create-global-mounts
echo "Setting permissions on hugeltbfs mounts"
chgrp -R pda /var/lib/hugetlbfs/global/*
chmod -R g+rwx /var/lib/hugetlbfs/global/*

# Display hugepage status
echo ""
echo "Hugepages:"
hugeadm --pool-list
echo ""
echo "Use 'echo [number] > /sys/kernel/mm/hugepages/hugepages-[size]/nr_hugepages' to allocate hugepages manually"
echo "Or set a number in the following conf files and run the script again:"
echo "  echo [number] > $HUGEPAGES_2M_CONF"
echo "  echo [number] > $HUGEPAGES_1G_CONF"
