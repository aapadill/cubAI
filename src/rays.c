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
	bool		invisible;

	invisible = false;
	tex_x = ray->wall_x * ray->texture->width;
	if (tex_x >= (int)ray->texture->width)
		tex_x = ray->texture->width - 1;
	if (ray->is_door)
	{
		int offset = (int)(ray->door_progress * ray->texture->width);
		tex_x = (tex_x + offset) % ray->texture->width;
		if (tex_x < offset)
		{
			invisible = true;
			(void)invisible;
			ray->is_door = false;
			ray->texture = get_wall_texture(data, ray);
			ray->is_door = true; // Necessary?
		}
	}
	y = vis_start_y;
	while (y <= vis_end_y)
	{
		// Compute the texture y-coordinate using the original (unclipped) wall parameters
		tex_y = ((y - orig_start_y) * ray->texture->height) / wall_height;
		if (tex_y >= (int)ray->texture->height)
			tex_y = ray->texture->height - 1;
		color = get_texture_color(ray->texture, tex_x, tex_y);
		shaded_color = simple_shading(color, ray->distance);
		mlx_put_pixel(data->image, screen_x, y, shaded_color);
		y++;
	}
}
