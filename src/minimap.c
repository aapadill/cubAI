/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minimap.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djelacik <djelacik@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/04 16:37:41 by djelacik          #+#    #+#             */
/*   Updated: 2025/01/04 17:29:59 by djelacik         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cub3D.h"

typedef struct s_minimap
{
	int		tile_px;
	int		tiles;
	int		size_px;
	int		origin_x;
	int		origin_y;
	int		center_x;
	int		center_y;
	double	radius;
	double	cos_a;
	double	sin_a;
}	t_minimap;

static int	min_int(int a, int b)
{
	if (a < b)
		return (a);
	return (b);
}

static int	max_int(int a, int b)
{
	if (a > b)
		return (a);
	return (b);
}

static void	init_minimap(t_data *data, t_minimap *mm)
{
	int	tiles;
	double	rot;

	mm->tile_px = (int)(TILE_SIZE * MINIMAP_SCALE);
	if (mm->tile_px < 2)
		mm->tile_px = 2;
	tiles = MINIMAP_TILES;
	if (tiles < 5)
		tiles = 5;
	if (tiles % 2 == 0)
		tiles++;
	mm->tiles = tiles;
	mm->size_px = tiles * mm->tile_px;
	mm->origin_x = MINIMAP_PADDING;
	mm->origin_y = MINIMAP_PADDING;
	if (mm->origin_x + mm->size_px > data->width)
		mm->origin_x = data->width - mm->size_px;
	if (mm->origin_y + mm->size_px > data->height)
		mm->origin_y = data->height - mm->size_px;
	if (mm->origin_x < 0)
		mm->origin_x = 0;
	if (mm->origin_y < 0)
		mm->origin_y = 0;
	mm->center_x = mm->origin_x + mm->size_px / 2;
	mm->center_y = mm->origin_y + mm->size_px / 2;
	mm->radius = (double)tiles / 2.0;
	rot = data->player.angle + M_PI / 2.0;
	mm->cos_a = cos(rot);
	mm->sin_a = sin(rot);
}

static void	world_to_minimap(t_minimap *mm, t_data *data, double wx, double wy,
	double *sx, double *sy)
{
	double	dx;
	double	dy;
	double	rx;
	double	ry;

	dx = wx - data->player.x;
	dy = wy - data->player.y;
	rx = dx * mm->cos_a + dy * mm->sin_a;
	ry = -dx * mm->sin_a + dy * mm->cos_a;
	*sx = mm->center_x + rx * mm->tile_px;
	*sy = mm->center_y + ry * mm->tile_px;
}

static void	draw_square_clipped(mlx_image_t *image, int x, int y, int size,
	int color, int clip_x, int clip_y, int clip_size)
{
	int	i;
	int	j;
	int	start_x;
	int	start_y;
	int	end_x;
	int	end_y;

	start_x = max_int(x, clip_x);
	start_y = max_int(y, clip_y);
	end_x = min_int(x + size, clip_x + clip_size);
	end_y = min_int(y + size, clip_y + clip_size);
	if (end_x <= start_x || end_y <= start_y)
		return ;
	i = start_x;
	while (i < end_x)
	{
		j = start_y;
		while (j < end_y)
		{
			mlx_put_pixel(image, i, j, color);
			j++;
		}
		i++;
	}
}

void	draw_mini_map(t_data *data)
{
	t_minimap	mm;
	int			x;
	int			y;
	int			range;
	int			screen_x;
	int			screen_y;
	double		sx;
	double		sy;
	int			player_x;
	int			player_y;
	char		*row;
	int			row_len;
	char		cell;

	init_minimap(data, &mm);
	draw_square(data->image, mm.origin_x, mm.origin_y, mm.size_px, BLACK_COLOR);
	player_x = (int)data->player.x;
	player_y = (int)data->player.y;
	range = (int)(mm.radius * 1.5) + 2;
	y = player_y - range;
	while (y <= player_y + range)
	{
		if (y < 0 || y >= data->map.height)
		{
			y++;
			continue ;
		}
		row = data->map.grid[y];
		row_len = ft_strlen(row);
		x = player_x - range;
		while (x <= player_x + range)
		{
			world_to_minimap(&mm, data, (double)x, (double)y, &sx, &sy);
			screen_x = (int)sx;
			screen_y = (int)sy;
			if (x >= 0 && x < row_len)
			{
				cell = row[x];
				if (cell == '1')
					draw_square_clipped(data->image, screen_x, screen_y,
						mm.tile_px, BLUE_COLOR, mm.origin_x, mm.origin_y,
						mm.size_px);
				else if (cell == 'D' && is_wall(data, (double)x, (double)y))
					draw_square_clipped(data->image, screen_x, screen_y,
						mm.tile_px, GREEN_COLOR, mm.origin_x, mm.origin_y,
						mm.size_px);
				else
					draw_square_clipped(data->image, screen_x, screen_y,
						mm.tile_px, BLUE_COLOR_2, mm.origin_x, mm.origin_y,
						mm.size_px);
			}
			x++;
		}
		y++;
	}
}

void	draw_mini_player(t_data *data)
{
	t_minimap	mm;
	int			x;
	int			y;
	int			size;
	int			i;
	int			j;
	int			px;
	int			py;

	init_minimap(data, &mm);
	x = mm.center_x;
	y = mm.center_y;
	size = mm.tile_px / 2;
	if (size < 3)
		size = 3;
	if (size % 2 == 0)
		size++;
	i = -size / 2;
	while (i <= size / 2)
	{
		j = -size / 2;
		while (j <= size / 2)
		{
			px = x + i;
			py = y + j;
			if (px >= mm.origin_x && px < mm.origin_x + mm.size_px
				&& py >= mm.origin_y && py < mm.origin_y + mm.size_px)
				mlx_put_pixel(data->image, px, py, RED_COLOR);
			j++;
		}
		i++;
	}
}

double	normalize_angle(double angle)
{
	while (angle >= 2 * M_PI)
		angle -= 2 * M_PI;
	while (angle < 0)
		angle += 2 * M_PI;
	return (angle);
}

//limit angle to 0 - 2 * M_PI
//rays look more like waves wtf
//make this dynamic (portion of the map depending on player's position)
void	draw_mini_rays(t_data *data)
{
	t_minimap	mm;
	double	ray_x;
	double	ray_y;
	double	angle;
	double	max_angle;
	double	screen_x;
	double	screen_y;
	double	sx;
	double	sy;
	double	traveled;
	double	max_dist;

	init_minimap(data, &mm);
	//data->player.angle = normalize_angle(data->player.angle);
	//printf("player.angle: %f\n", data->player.angle);
	//angle = normalize_angle(data->player.angle - FOV / 2);
	angle = data->player.angle - FOV / 2;
	//max_angle = normalize_angle(data->player.angle + FOV / 2);
	max_angle = data->player.angle + FOV / 2;
	max_dist = mm.radius;
	//assert(data->player.angle >= 0 && data->player.angle < 2 * M_PI);
	while (angle <= max_angle)
	{
		//assert(angle >= 0 && angle < 2 * M_PI);
		ray_x = data->player.x;
		ray_y = data->player.y;
		traveled = 0.0;
		while (!is_wall(data, ray_x, ray_y) && traveled < max_dist)
		{
			//assert(ray_x >= 0 && ray_x < data->map.width);
			//assert(ray_y >= 0 && ray_y < data->map.height);
			ray_x += cos(angle) * STEP_SIZE;
			ray_y += sin(angle) * STEP_SIZE;
			traveled += STEP_SIZE;
			world_to_minimap(&mm, data, ray_x, ray_y, &sx, &sy);
			screen_x = sx;
			screen_y = sy;
			if (screen_x >= mm.origin_x && screen_x < mm.origin_x + mm.size_px
				&& screen_y >= mm.origin_y && screen_y < mm.origin_y + mm.size_px)
				mlx_put_pixel(data->image, (int)screen_x, (int)screen_y,
					YELLOW_COLOR);
		}
		angle += 0.01;
		//angle = normalize_angle(angle);
	}
}

void	draw_square(mlx_image_t *image, int x, int y, int size, int color)
{
	int	i;
	int	j;

	i = 0;
	while (i < size)
	{
		j = 0;
		while (j < size)
		{
			// Piirrä ilman reunoja
			mlx_put_pixel(image, x + i, y + j, color);
			j++;
		}
		i++;
	}
}
