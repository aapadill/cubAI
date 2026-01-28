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

void draw_sprite_column(t_data *data, int x, double dir_x, double dir_y, double plane_x, double plane_y)
{
    // Local helper structure for per-column sprite info
    typedef struct s_sprite_proj {
        int     index;
        double  transform_y;      // sprite depth in camera space
        int     sprite_screen_x;  // sprite center on screen (x coordinate)
        int     sprite_width;
        int     sprite_height;
        int     draw_start_x;
        int     draw_end_x;
        int     draw_start_y;
        int     draw_end_y;
    } t_sprite_proj;

    t_sprite_proj colSprites[data->num_sprites];
    int colSpriteCount = 0;

    // Iterate over all sprites to see which affect column x
    for (int i = 0; i < data->num_sprites; i++)
    {
        double sprite_dx = data->sprites[i].x - data->player.x;
        double sprite_dy = data->sprites[i].y - data->player.y;

        double inv_det = 1.0 / (plane_x * dir_y - dir_x * plane_y);
        double transform_x = inv_det * (dir_y * sprite_dx - dir_x * sprite_dy);
        double transform_y = inv_det * (-plane_y * sprite_dx + plane_x * sprite_dy);

        // Skip sprites behind the camera
        if (transform_y <= 0)
            continue;

        int sprite_screen_x = (int)(((double)data->width / 2.0) * (1 + transform_x / transform_y));

        // Calculate sprite size and vertical drawing bounds
        int sprite_height = abs((int)(data->height / transform_y));
        int draw_start_y = -sprite_height / 2 + data->height / 2;
        if (draw_start_y < 0)
            draw_start_y = 0;
        int draw_end_y = sprite_height / 2 + data->height / 2;
        if (draw_end_y >= data->height)
            draw_end_y = data->height - 1;

        // Calculate horizontal sprite dimensions
        int sprite_width = abs((int)(data->height / transform_y));
        int draw_start_x = -sprite_width / 2 + sprite_screen_x;
        if (draw_start_x < 0)
            draw_start_x = 0;
        int draw_end_x = sprite_width / 2 + sprite_screen_x;
        if (draw_end_x >= data->width)
            draw_end_x = data->width - 1;

        // If the current column is within the sprite's horizontal span, store its info
        if (x >= draw_start_x && x < draw_end_x)
        {
            colSprites[colSpriteCount].index = i;
            colSprites[colSpriteCount].transform_y = transform_y;
            colSprites[colSpriteCount].sprite_screen_x = sprite_screen_x;
            colSprites[colSpriteCount].sprite_width = sprite_width;
            colSprites[colSpriteCount].sprite_height = sprite_height;
            colSprites[colSpriteCount].draw_start_x = draw_start_x;
            colSprites[colSpriteCount].draw_end_x = draw_end_x;
            colSprites[colSpriteCount].draw_start_y = draw_start_y;
            colSprites[colSpriteCount].draw_end_y = draw_end_y;
            colSpriteCount++;
        }
    }

    // Sort the collected sprites for this column (farthest first)
    for (int i = 0; i < colSpriteCount - 1; i++)
    {
        for (int j = 0; j < colSpriteCount - i - 1; j++)
        {
            if (colSprites[j].transform_y < colSprites[j + 1].transform_y)
            {
                t_sprite_proj tmp = colSprites[j];
                colSprites[j] = colSprites[j + 1];
                colSprites[j + 1] = tmp;
            }
        }
    }

    // Draw each sprite affecting this column
    for (int i = 0; i < colSpriteCount; i++)
    {
        t_sprite_proj *sp = &colSprites[i];
        mlx_texture_t *tex = data->sprite_textures[data->sprites[sp->index].texture];

        int tex_x = (int)(256 * (x - (-sp->sprite_width / 2 + sp->sprite_screen_x)) * tex->width / sp->sprite_width) / 256;

        for (int y = sp->draw_start_y; y < sp->draw_end_y; y++)
        {
            int d = y * 256 - data->height * 128 + sp->sprite_height * 128;
            int tex_y = ((d * tex->height) / sp->sprite_height) / 256;
            uint32_t color = get_texture_color(tex, tex_x, tex_y);
            uint8_t alpha = (color >> 24) & 0xFF;

            // Draw only if pixel is visible (alpha) and sprite is in front
            if (alpha != 0 && sp->transform_y < data->zBuffer[x])
            {
				uint32_t fixed_color = simple_shading(color, sp->transform_y);
                mlx_put_pixel(data->image, x, y, fixed_color);
            }
        }
    }
}
