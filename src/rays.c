/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rays.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djelacik <djelacik@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/04 16:29:26 by djelacik          #+#    #+#             */
/*   Updated: 2025/01/05 22:34:48 by djelacik         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cub3D.h"

void	draw_walls_and_sprites(t_data *data)
{
	double	dir_x;
	double	dir_y;
	double	plane_x;
	double	plane_y;
	int		screen_x;

	dir_x = cos(data->player.angle);
	dir_y = sin(data->player.angle);
	plane_x = -dir_y * tan(FOV / 2.0);
	plane_y = dir_x * tan(FOV / 2.0);
	screen_x = 0;
	while (screen_x < data->width)
	{
		double	camera_x;
		double	ray_dir_x;
		double	ray_dir_y;

		camera_x = 2.0 * screen_x / (double)data->width - 1.0;
		ray_dir_x = dir_x + plane_x * camera_x;
		ray_dir_y = dir_y + plane_y * camera_x;
		draw_wall_column(data, ray_dir_x, ray_dir_y, screen_x);
		screen_x++;
	}
	draw_sprites(data, dir_x, dir_y, plane_x, plane_y);
}

void	draw_wall_column(t_data *data, double ray_dir_x, double ray_dir_y, int screen_x)
{
	t_ray	ray;
	int		orig_start_y;
	int		orig_end_y;
	int		vis_start_y;
	int		vis_end_y;
	int		wall_height;
	int		center_y;

	// Calculate ray properties
	calculate_ray_data(data, ray_dir_x, ray_dir_y, &ray);
	data->zBuffer[screen_x] = ray.distance;

	// Compute the full (virtual) wall height and its starting/ending positions
	wall_height = (int)(data->height / ray.distance);
	//orig_start_y = (data->height / 2) - (wall_height / 2);
	//orig_end_y = (data->height / 2) + (wall_height / 2);
	center_y = (int)(data->height / 2 + data->camera.shake_offset);
	orig_start_y = center_y - (wall_height / 2);
	orig_end_y = center_y + (wall_height / 2);

	// Clip the drawing area to the visible screen (but keep the original positions for texture mapping)
	vis_start_y = orig_start_y;
	vis_end_y = orig_end_y;
	if (vis_start_y < 0)
		vis_start_y = 0;
	if (vis_end_y >= data->height)
		vis_end_y = data->height - 1;

	// Pass both the visible drawing range and the original wall info
	draw_wall_texture(data, &ray, screen_x, vis_start_y, vis_end_y, orig_start_y, wall_height);
}

void	draw_wall_texture(t_data *data, t_ray *ray, int screen_x, int vis_start_y, int vis_end_y, int orig_start_y, int wall_height)
{
	int			y;
	int			tex_x;
	int			tex_y;
	uint32_t	color;
	uint32_t	shaded_color;
	double		tex_pos;
	double		tex_step;
	double		factor;
	int			tex_h;
	int			tex_w;
	double		door_progress;
	double		local_x;
	bool		is_door;

	is_door = ray->is_door;
	tex_h = (int)ray->texture->height;
	tex_w = (int)ray->texture->width;
	if (is_door)
	{
		door_progress = ray->door_progress;
		if (door_progress < 0.0)
			door_progress = 0.0;
		else if (door_progress > 1.0)
			door_progress = 1.0;
		local_x = ray->wall_x - door_progress;
		if (local_x < 0.0)
			local_x = 0.0;
		else if (local_x > 1.0)
			local_x = 1.0;
		tex_x = (int)(local_x * tex_w);
		if (tex_x >= tex_w)
			tex_x = tex_w - 1;
	}
	else
	{
		tex_x = ray->wall_x * tex_w;
		if (tex_x >= tex_w)
			tex_x = tex_w - 1;
	}
	factor = ray->distance * 0.4;
	if (factor < 1.0)
		factor = 1.0;
	else
		factor = 1.0 / factor;
	tex_step = (double)tex_h / (double)wall_height;
	tex_pos = (vis_start_y - orig_start_y) * tex_step;
	y = vis_start_y;
	while (y <= vis_end_y)
	{
		tex_y = (int)tex_pos;
		if (tex_y >= tex_h)
			tex_y = tex_h - 1;
		color = get_texture_color(ray->texture, tex_x, tex_y);
		shaded_color = get_rgba(
				(uint8_t)(get_r(color) * factor),
				(uint8_t)(get_g(color) * factor),
				(uint8_t)(get_b(color) * factor),
				get_a(color));
		mlx_put_pixel(data->image, screen_x, y, shaded_color);
		tex_pos += tex_step;
		y++;
	}
}
