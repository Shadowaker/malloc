#include "malloc.h"

size_t	get_page_size(void)
{
	static size_t	ps = 0;

	if (!ps)
		ps = (size_t)sysconf(_SC_PAGESIZE);

	return (ps);
}

size_t	get_zone_size(size_t max_block_size)
{
	size_t	ps;
	size_t	min_bytes;

	ps = get_page_size();
	min_bytes = sizeof(t_zone) + 100 * (sizeof(t_block) + max_block_size);
	return ((min_bytes + ps - 1) / ps * ps);
}

void	write_hex(unsigned long n)
{
	char	digits[] = "0123456789abcdef";
	char	buf[16];
	int		len;

	write(1, "0x", 2);

	if (n == 0)
	{
		write(1, "0", 1);
		return ;
	}

	len = 0;
	while (n)
	{
		buf[len++] = digits[n & 0xf];
		n >>= 4;
	}

	while (len > 0)
		write(1, &buf[--len], 1);
}

void	write_size(size_t n)
{
	char	buf[20];
	int		len;

	if (n == 0)
	{
		write(1, "0", 1);
		return ;
	}

	len = 0;
	while (n)
	{
		buf[len++] = '0' + (n % 10);
		n /= 10;
	}

	while (len > 0)
		write(1, &buf[--len], 1);
}
