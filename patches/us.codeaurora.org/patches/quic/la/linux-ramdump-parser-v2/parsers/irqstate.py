# Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

from print_out import print_out_str
from parser_util import register_parser, RamParser


@register_parser('--print-irqs', 'Print all the irq information', shortopt='-i')
class IrqParse(RamParser):

    def print_irq_state_3_0(self, ram_dump):
        print_out_str(
            '=========================== IRQ STATE ===============================')
        per_cpu_offset_addr = ram_dump.addr_lookup('__per_cpu_offset')
        cpu_present_bits_addr = ram_dump.addr_lookup('cpu_present_bits')
        cpu_present_bits = ram_dump.read_word(cpu_present_bits_addr)
        cpus = bin(cpu_present_bits).count('1')
        irq_desc = ram_dump.addr_lookup('irq_desc')
        foo, irq_desc_size = ram_dump.unwind_lookup(irq_desc, 1)
        h_irq_offset = ram_dump.field_offset('struct irq_desc', 'handle_irq')
        irq_num_offset = ram_dump.field_offset('struct irq_data', 'irq')
        irq_data_offset = ram_dump.field_offset('struct irq_desc', 'irq_data')
        irq_count_offset = ram_dump.field_offset(
            'struct irq_desc', 'irq_count')
        irq_chip_offset = ram_dump.field_offset('struct irq_data', 'chip')
        irq_action_offset = ram_dump.field_offset('struct irq_desc', 'action')
        action_name_offset = ram_dump.field_offset('struct irqaction', 'name')
        kstat_irqs_offset = ram_dump.field_offset(
            'struct irq_desc', 'kstat_irqs')
        chip_name_offset = ram_dump.field_offset('struct irq_chip', 'name')
        irq_desc_entry_size = ram_dump.sizeof('irq_desc[0]')
        cpu_str = ''

        for i in range(0, cpus):
            cpu_str = cpu_str + '{0:10} '.format('CPU{0}'.format(i))

        print_out_str(
            '{0:4} {1} {2:30} {3:10}'.format('IRQ', cpu_str, 'Name', 'Chip'))
        for i in range(0, irq_desc_size, irq_desc_entry_size):
            irqnum = ram_dump.read_word(irq_desc + i + irq_num_offset)
            irqcount = ram_dump.read_word(irq_desc + i + irq_count_offset)
            action = ram_dump.read_word(irq_desc + i + irq_action_offset)
            kstat_irqs_addr = ram_dump.read_word(
                irq_desc + i + kstat_irqs_offset)
            irq_stats_str = ''

            for j in range(0, cpus):
                if per_cpu_offset_addr is None:
                    offset = 0
                else:
                    offset = ram_dump.read_word(per_cpu_offset_addr + 4 * j)
                irq_statsn = ram_dump.read_word(kstat_irqs_addr + offset)
                irq_stats_str = irq_stats_str + \
                    '{0:10} '.format('{0}'.format(irq_statsn))

            chip = ram_dump.read_word(
                irq_desc + i + irq_data_offset + irq_chip_offset)
            chip_name_addr = ram_dump.read_word(chip + chip_name_offset)
            chip_name = ram_dump.read_cstring(chip_name_addr, 48)

            if action != 0:
                name_addr = ram_dump.read_word(action + action_name_offset)
                name = ram_dump.read_cstring(name_addr, 48)
                print_out_str(
                    '{0:4} {1} {2:30} {3:10}'.format(irqnum, irq_stats_str, name, chip_name))

    def radix_tree_lookup_element(self, ram_dump, root_addr, index):
        rnode_offset = ram_dump.field_offset('struct radix_tree_root', 'rnode')
        rnode_height_offset = ram_dump.field_offset(
            'struct radix_tree_node', 'height')
        slots_offset = ram_dump.field_offset('struct radix_tree_node', 'slots')
        pointer_size = ram_dump.sizeof('struct radix_tree_node *')

        # if CONFIG_BASE_SMALL=0: radix_tree_map_shift = 6
        radix_tree_map_shift = 6
        radix_tree_map_mask = 0x3f
        height_to_maxindex = [0x0, 0x3F, 0x0FFF,
                              0x0003FFFF, 0x00FFFFFF, 0x3FFFFFFF, 0xFFFFFFFF]

        if ram_dump.read_word(root_addr + rnode_offset) & 1 == 0:
            if index > 0:
                return None
            return (ram_dump.read_word(root_addr + rnode_offset) & 0xfffffffffffffffe)

        node_addr = ram_dump.read_word(root_addr + rnode_offset) & 0xfffffffffffffffe
        height = ram_dump.read_int(node_addr + rnode_height_offset)

        if height > len(height_to_maxindex):
            return None

        if index > height_to_maxindex[height]:
            return None

        shift = (height - 1) * radix_tree_map_shift
        for h in range(height, 0, -1):
            node_addr = ram_dump.read_word(
                node_addr + slots_offset + ((index >> shift) & radix_tree_map_mask) * pointer_size)
            if node_addr == 0:
                return None
            shift -= radix_tree_map_shift
        return (node_addr & 0xfffffffffffffffe)

    def print_irq_state_sparse_irq(self, ram_dump):
        h_irq_offset = ram_dump.field_offset('struct irq_desc', 'handle_irq')
        irq_num_offset = ram_dump.field_offset('struct irq_data', 'irq')
        irq_data_offset = ram_dump.field_offset('struct irq_desc', 'irq_data')
        irq_count_offset = ram_dump.field_offset(
            'struct irq_desc', 'irq_count')
        irq_chip_offset = ram_dump.field_offset('struct irq_data', 'chip')
        irq_action_offset = ram_dump.field_offset('struct irq_desc', 'action')
        action_name_offset = ram_dump.field_offset('struct irqaction', 'name')
        kstat_irqs_offset = ram_dump.field_offset(
            'struct irq_desc', 'kstat_irqs')
        chip_name_offset = ram_dump.field_offset('struct irq_chip', 'name')
        cpu_str = ''

        irq_desc_tree = ram_dump.addr_lookup('irq_desc_tree')
        nr_irqs = ram_dump.read_int(ram_dump.addr_lookup('nr_irqs'))

        for i in ram_dump.iter_cpus():
            cpu_str = cpu_str + '{0:10} '.format('CPU{0}'.format(i))

        print_out_str(
            '{0:4} {1} {2:30} {3:10}'.format('IRQ', cpu_str, 'Name', 'Chip'))

        if nr_irqs > 50000:
            return

        for i in range(0, nr_irqs):
            irq_desc = self.radix_tree_lookup_element(
                ram_dump, irq_desc_tree, i)
            if irq_desc is None:
                continue
            irqnum = ram_dump.read_word(irq_desc + irq_num_offset)
            irqcount = ram_dump.read_word(irq_desc + irq_count_offset)
            action = ram_dump.read_word(irq_desc + irq_action_offset)
            kstat_irqs_addr = ram_dump.read_word(irq_desc + kstat_irqs_offset)
            irq_stats_str = ''

            if kstat_irqs_addr is None:
                break

            for j in ram_dump.iter_cpus():
                irq_statsn = ram_dump.read_int(kstat_irqs_addr, cpu=j)
                irq_stats_str = irq_stats_str + \
                    '{0:10} '.format('{0}'.format(irq_statsn))

            chip = ram_dump.read_word(
                irq_desc + irq_data_offset + irq_chip_offset)
            chip_name_addr = ram_dump.read_word(chip + chip_name_offset)
            chip_name = ram_dump.read_cstring(chip_name_addr, 48)

            if action != 0:
                name_addr = ram_dump.read_word(action + action_name_offset)
                name = ram_dump.read_cstring(name_addr, 48)
                print_out_str(
                    '{0:4} {1} {2:30} {3:10}'.format(irqnum, irq_stats_str, name, chip_name))

    def parse(self):
        irq_desc = self.ramdump.addr_lookup('irq_desc')
        if self.ramdump.is_config_defined('CONFIG_SPARSE_IRQ'):
            self.print_irq_state_sparse_irq(self.ramdump)

        if irq_desc is None:
            return

        self.print_irq_state_3_0(self.ramdump)
