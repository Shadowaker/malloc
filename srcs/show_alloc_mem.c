#include "malloc.h"

static size_t	print_zone_list(t_zone *zone, char *label, int label_len)
{
	t_block	*block;
	void	*start;
	void	*end;
	size_t	total;

	total = 0;
	while (zone)
	{

		write(1, label, label_len);
		write_hex((unsigned long)zone);
		write(1, "\n", 1);

		block = (t_block *)(zone + 1);

		while (block)
		{
			if (!block->free)
			{
				start = (void *)(block + 1);
				end = (char *)start + block->size;

				write_hex((unsigned long)start);
				write(1, " - ", 3);
				write_hex((unsigned long)end);
				write(1, " : ", 3);
				write_size(block->size);
				write(1, " bytes\n", 7);

				total += block->size;
			}
			block = block->next;
		}
		zone = zone->next;
	}

	return (total);
}

static size_t	print_zone_list_ex(t_zone *zone, char *label, int label_len)
{
	t_block	*block;
	void	*start;
	void	*end;
	size_t	total;

	total = 0;
	while (zone)
	{
		write(1, label, label_len);
		write_hex((unsigned long)zone);
		write(1, "\n", 1);
		block = (t_block *)(zone + 1);
		while (block)
		{
			start = (void *)(block + 1);
			end = (char *)start + block->size;
			write_hex((unsigned long)start);
			write(1, " - ", 3);
			write_hex((unsigned long)end);
			write(1, " : ", 3);
			write_size(block->size);
			if (block->free)
				write(1, " bytes (free)\n", 14);
			else
			{
				write(1, " bytes\n", 7);
				write_hex_dump(start, block->size);
				total += block->size;
			}
			block = block->next;
		}
		zone = zone->next;
	}
	return (total);
}

void	show_alloc_mem(void)
{
	size_t	total;

	total = 0;

	total += print_zone_list(g_heap.tiny, "TINY : ", 7);
	total += print_zone_list(g_heap.small, "SMALL : ", 8);
	total += print_zone_list(g_heap.large, "LARGE : ", 8);

	write(1, "Total : ", 8);
	write_size(total);
	write(1, " bytes\n", 7);
}

void	show_alloc_mem_ex(void)
{
	size_t	total;

	total = 0;
	total += print_zone_list_ex(g_heap.tiny, "TINY : ", 7);
	total += print_zone_list_ex(g_heap.small, "SMALL : ", 8);
	total += print_zone_list_ex(g_heap.large, "LARGE : ", 8);
	write(1, "Total : ", 8);
	write_size(total);
	write(1, " bytes\n", 7);
}