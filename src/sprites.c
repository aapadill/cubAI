/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sprites.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aapadill <aapadill@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/08 22:41:52 by aapadill          #+#    #+#             */
/*   Updated: 2025/03/08 22:42:09 by aapadill         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cub3D.h"

void	draw_sprites(t_data *data, double dir_x, double dir_y, double plane_x, double plane_y)
{
	typedef struct s_sprite_proj {
		int		index;
		double	transform_y;
		int		sprite_screen_x;
		int		sprite_width;
		int		sprite_height;
		int		draw_start_x;
		int		draw_end_x;
		int		draw_start_y;
		int		draw_end_y;
	} t_sprite_proj;

	int				num_sprites;
	int				count;
	double			inv_det;

	num_sprites = data->num_sprites;
	if (num_sprites <= 0)
		return ;
	t_sprite_proj	sprites[num_sprites];
	inv_det = plane_x * dir_y - dir_x * plane_y;
	if (inv_det == 0.0)
		return ;
	inv_det = 1.0 / inv_det;
	count = 0;
	for (int i = 0; i < num_sprites; i++)
	{
		double sprite_dx = data->sprites[i].x - data->player.x;
		double sprite_dy = data->sprites[i].y - data->player.y;
		double transform_x = inv_det * (dir_y * sprite_dx - dir_x * sprite_dy);
		double transform_y = inv_det * (-plane_y * sprite_dx + plane_x * sprite_dy);

		if (transform_y <= 0)
			continue;
		int sprite_screen_x = (int)(((double)data->width / 2.0)
				* (1 + transform_x / transform_y));
		int sprite_height = abs((int)(data->height / transform_y));
		int draw_start_y = -sprite_height / 2 + data->height / 2;
		if (draw_start_y < 0)
			draw_start_y = 0;
		int draw_end_y = sprite_height / 2 + data->height / 2;
		if (draw_end_y >= data->height)
			draw_end_y = data->height - 1;
		int sprite_width = abs((int)(data->height / transform_y));
		int draw_start_x = -sprite_width / 2 + sprite_screen_x;
		if (draw_start_x < 0)
			draw_start_x = 0;
		int draw_end_x = sprite_width / 2 + sprite_screen_x;
		if (draw_end_x >= data->width)
			draw_end_x = data->width - 1;
		sprites[count].index = i;
		sprites[count].transform_y = transform_y;
		sprites[count].sprite_screen_x = sprite_screen_x;
		sprites[count].sprite_width = sprite_width;
		sprites[count].sprite_height = sprite_height;
		sprites[count].draw_start_x = draw_start_x;
		sprites[count].draw_end_x = draw_end_x;
		sprites[count].draw_start_y = draw_start_y;
		sprites[count].draw_end_y = draw_end_y;
		count++;
	}
	for (int i = 0; i < count - 1; i++)
	{
		for (int j = 0; j < count - i - 1; j++)
		{
			if (sprites[j].transform_y < sprites[j + 1].transform_y)
			{
				t_sprite_proj tmp = sprites[j];
				sprites[j] = sprites[j + 1];
				sprites[j + 1] = tmp;
			}
		}
	}
	for (int i = 0; i < count; i++)
	{
		t_sprite_proj *sp = &sprites[i];
		mlx_texture_t *tex = data->sprite_textures[data->sprites[sp->index].texture];
		for (int x = sp->draw_start_x; x < sp->draw_end_x; x++)
		{
			int tex_x = (int)(256 * (x - (-sp->sprite_width / 2
							+ sp->sprite_screen_x)) * tex->width / sp->sprite_width) / 256;
			if (sp->transform_y >= data->zBuffer[x])
				continue;
			for (int y = sp->draw_start_y; y < sp->draw_end_y; y++)
			{
				int d = y * 256 - data->height * 128 + sp->sprite_height * 128;
				int tex_y = ((d * tex->height) / sp->sprite_height) / 256;
				uint32_t color = get_texture_color(tex, tex_x, tex_y);
				uint8_t alpha = (color >> 24) & 0xFF;

				if (alpha != 0)
				{
					uint32_t fixed_color = simple_shading(color, sp->transform_y);
					mlx_put_pixel(data->image, x, y, fixed_color);
				}
			}
		}
	}
}
