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

static void	write_hex_byte(unsigned char b)
{
	char	hex[] = "0123456789abcdef";
	char	buf[2];

	buf[0] = hex[b >> 4];
	buf[1] = hex[b & 0xf];
	write(1, buf, 2);
}

void	write_hex_dump(void *ptr, size_t size)
{
	unsigned char	*p;
	size_t			i;
	size_t			j;
	size_t			n;
	char			c;

	p = (unsigned char *)ptr;
	i = 0;
	while (i < size)
	{
		n = size - i < 16 ? size - i : 16;
		write_hex((unsigned long)(p + i));
		write(1, ": ", 2);
		j = 0;
		while (j < n)
		{
			write_hex_byte(p[i + j]);
			write(1, " ", 1);
			j++;
		}
		while (j++ < 16)
			write(1, "   ", 3);
		write(1, " ", 1);
		j = 0;
		while (j < n)
		{
			c = (p[i + j] >= 32 && p[i + j] < 127) ? (char)p[i + j] : '.';
			write(1, &c, 1);
			j++;
		}
		write(1, "\n", 1);
		i += n;
	}
}