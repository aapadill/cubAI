/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ray.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aapadill <aapadill@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/09 21:24:42 by aapadill          #+#    #+#             */
/*   Updated: 2025/02/21 18:00:30 by aapadill         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cub3D.h"

void	calculate_ray_data(t_data *data, double ray_dir_x, double ray_dir_y, t_ray *ray)
{
	double delta_dist_x;
	double delta_dist_y;
	double side_dist_x;
	double side_dist_y;
	int map_x;
	int map_y;
	int step_x;
	int step_y;

	ray->dir_x = ray_dir_x;
	ray->dir_y = ray_dir_y;

	map_x = (int)data->player.x;
	map_y = (int)data->player.y;

	if (ray->dir_x == 0)
		delta_dist_x = 1e30; //to avoid division by zero
	else
		delta_dist_x = fabs(1 /ray->dir_x);
	if (ray->dir_y == 0)
		delta_dist_y = 1e30; //to avoid division by zero
	else
		delta_dist_y = fabs(1 / ray->dir_y);

	if (ray->dir_x < 0)
	{
		step_x = -1;
		side_dist_x = (data->player.x - map_x) * delta_dist_x;
	}
	else
	{
		step_x = 1;
		side_dist_x = (map_x + 1.0 - data->player.x) * delta_dist_x;
	}
	if (ray->dir_y < 0)
	{
		step_y = -1;
		side_dist_y = (data->player.y - map_y) * delta_dist_y;
	}
	else
	{
		step_y = 1;
		side_dist_y = (map_y + 1.0 - data->player.y) * delta_dist_y;
	}
	ray->is_door = false;
	ray->door_progress = 0;
	// DDA: Eteneminen gridissä
	while (1)
	{
		if (side_dist_x < side_dist_y)
		{
			side_dist_x += delta_dist_x;
			map_x += step_x;
			ray->side = 0;
		}
		else
		{
			side_dist_y += delta_dist_y;
			map_y += step_y;
			ray->side = 1;
		}
		char cell = data->map.grid[map_y][map_x];
		if (cell == '1') //stop ray, this is a wall
			break;
		else if (cell == 'D') //this is a door
		{
			ray->door_progress = lookup_door_progress(data, map_x, map_y);
			if (ray->door_progress <= 1)
			{
				ray->is_door = true;
				break;
			}
		}
	}
	if (ray->side == 0)
	{
		//ray->hit_x = map_x;
		ray->distance = (map_x - data->player.x + (1 - step_x) / 2) / ray->dir_x;
		ray->hit_y = data->player.y + ray->distance * ray->dir_y;
		ray->wall_x = ray->hit_y - floor(ray->hit_y);
	}
	else
	{
		//ray->hit_y = map_y;
		ray->distance = (map_y - data->player.y + (1 - step_y) / 2) / ray->dir_y;
		ray->hit_x = data->player.x + ray->distance * ray->dir_x;
		ray->wall_x = ray->hit_x - floor(ray->hit_x);
	}
	ray->texture = get_wall_texture(data, ray);
}
