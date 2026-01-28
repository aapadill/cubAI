/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   movement.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djelacik <djelacik@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/31 15:40:09 by djelacik          #+#    #+#             */
/*   Updated: 2026/01/28 04:42:43 by aapadill         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cub3D.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// void wrapper(void *param)
// {
// 	t_data *data = (t_data *)param;

// 	if (data->game_state == STATE_MENU)
// 	{
// 		entry_screen(data);
// 	}
// 	else if (data->game_state == STATE_PLAYING)
// 	{
// 		loop_hook(data);
// 	}
// }

//try to load *all* frames into the aux array.
//if a frame isn’t yet on disk, we wait a bit and try again next frame
static void try_load_hands(t_data *data)
{
    char path[PATH_MAX];
    bool all_loaded = true;

    for (int i = 0; i < data->hud_frame_count; ++i)
    {
        if (data->ai_hands[i] == NULL)  //not loaded yet
        {
            snprintf(path, sizeof(path), "textures/hand/handx%d.png", i + 1);

            if (access(path, F_OK) == 0)
            {
                data->ai_hands[i] = mlx_load_png(path);
                if (data->ai_hands[i])
                    printf("Loaded %s\n", path);
                else
                    fprintf(stderr, "Failed to load %s\n", path);
            }
            else
            {
                all_loaded = false;
				data->calling_new_gun = true;
     			data->is_gun_ready    = false;
            }
        }
    }
    if (all_loaded)
    {
        //swap in the new textures
        for (int i = 0; i < data->hud_frame_count; ++i)
        {
            //promote new
			if (data->ai_hands[i])
			{
				//using an auxiliar pointer to delete the old texture
				mlx_texture_t *old = data->hud_hands[i];
				snprintf(path, sizeof(path), "textures/hand/handx%d.png", i + 1);
				data->hud_hands[i] = mlx_load_png(path);
				mlx_delete_texture(data->ai_hands[i]);
				data->ai_hands[i] = NULL;
				mlx_delete_texture(old);
			}
        }
        data->calling_new_gun = false;
        data->is_gun_ready    = true;
        printf("New gun is ready!\n");
    }
}

static int can_move_to(t_data *data, double new_x, double new_y)
{
	// Check four diagonal directions
	const double	diag_offset = COLLISION_LIMIT * 0.7071067811865476;

	// Check four cardinal directions
	if (is_wall(data, new_x + COLLISION_LIMIT, new_y)) // Right
		return 0;
	if (is_wall(data, new_x - COLLISION_LIMIT, new_y)) // Left
		return 0;
	if (is_wall(data, new_x, new_y + COLLISION_LIMIT)) // Down
		return 0;
	if (is_wall(data, new_x, new_y - COLLISION_LIMIT)) // Up
		return 0;
	if (is_wall(data, new_x + diag_offset, new_y + diag_offset)) // Bottom-right
		return 0;
	if (is_wall(data, new_x - diag_offset, new_y + diag_offset)) // Bottom-left
		return 0;
	if (is_wall(data, new_x + diag_offset, new_y - diag_offset)) // Top-right
		return 0;
	if (is_wall(data, new_x - diag_offset, new_y - diag_offset)) // Top-left
		return 0;
	return 1; // No collisions, movement allowed
}

//normalize diagonal movement
static void handle_movement(t_data *data)
{
	double orig_x;
	double orig_y;
	double new_x;
	double new_y;
	double dir_x;
	double dir_y;
	double right_x;
	double right_y;
	double move_x;
	double move_y;
	double len;
	int input_f;
	int input_s;

	orig_x = data->player.x;
	orig_y = data->player.y;
	input_f = 0;
	input_s = 0;
	if (mlx_is_key_down(data->mlx, MLX_KEY_W))
		input_f += 1;
	if (mlx_is_key_down(data->mlx, MLX_KEY_S))
		input_f -= 1;
	if (mlx_is_key_down(data->mlx, MLX_KEY_D))
		input_s += 1;
	if (mlx_is_key_down(data->mlx, MLX_KEY_A))
		input_s -= 1;
	if (input_f == 0 && input_s == 0)
	{
		data->is_player_moving = false;
		return;
	}
	dir_x = cos(data->player.angle);
	dir_y = sin(data->player.angle);
	right_x = -dir_y;
	right_y = dir_x;
	move_x = dir_x * input_f + right_x * input_s;
	move_y = dir_y * input_f + right_y * input_s;
	len = sqrt(move_x * move_x + move_y * move_y);
	if (len == 0.0)
	{
		data->is_player_moving = false;
		return;
	}
	move_x = (move_x / len) * data->player.speed;
	move_y = (move_y / len) * data->player.speed;
	new_x = orig_x + move_x;
	new_y = orig_y + move_y;

	// Try moving diagonally first
	data->is_player_moving = false;
	if (can_move_to(data, new_x, new_y))
	{
		data->player.x = new_x;
		data->player.y = new_y;
		data->is_player_moving = true;
	}
	else
	{
		// If blocked, try moving along X only
		if (can_move_to(data, new_x, orig_y))
		{
			data->player.x = new_x;
			data->is_player_moving = true;
		}
		// And try moving along Y only
		if (can_move_to(data, orig_x, new_y))
		{
			data->player.y = new_y;
			data->is_player_moving = true;
		}
	}
}

static int	enemy_dir_from_move(t_data *data, double vx, double vy)
{
	double	dir_x;
	double	dir_y;
	double	right_x;
	double	right_y;
	double	dot_f;
	double	dot_r;

	dir_x = cos(data->player.angle);
	dir_y = sin(data->player.angle);
	right_x = -dir_y;
	right_y = dir_x;
	dot_f = vx * dir_x + vy * dir_y;
	dot_r = vx * right_x + vy * right_y;
	if (fabs(dot_f) >= fabs(dot_r))
	{
		if (dot_f < 0.0)
			return (ENEMY_DIR_FRONT);
		return (ENEMY_DIR_BACK);
	}
	if (dot_r > 0.0)
		return (ENEMY_DIR_RIGHT);
	return (ENEMY_DIR_LEFT);
}

static void	update_enemy_anim(t_data *data, double vx, double vy)
{
	double	len2;

	if (!data->enemy_anim_enabled)
		return ;
	len2 = vx * vx + vy * vy;
	if (len2 < 1e-6)
	{
		data->enemy_anim_frame = 0;
		data->enemy_anim_timer = 0;
		return ;
	}
	if (data->enemy_anim_dirs <= 1 || ENEMY_ANIM_FRONT_ONLY)
		data->enemy_anim_dir = ENEMY_DIR_FRONT;
	else
		data->enemy_anim_dir = enemy_dir_from_move(data, vx, vy);
	data->enemy_anim_timer++;
	if (data->enemy_anim_timer >= ENEMY_ANIM_TICK)
	{
		data->enemy_anim_timer = 0;
		data->enemy_anim_frame++;
		if (data->enemy_anim_frame >= data->enemy_anim_frames)
			data->enemy_anim_frame = 0;
	}
}

static void handle_enemy(t_data *data)
{
    // assume sprite 0 is the enemy
    double *ex = &data->sprites[0].x;
    double *ey = &data->sprites[0].y;
	double	prev_x;
	double	prev_y;

    // vector from enemy to player
    double dx = data->player.x - *ex;
    double dy = data->player.y - *ey;

	prev_x = *ex;
	prev_y = *ey;
    double dist2 = dx * dx + dy * dy;
    if (dist2 < 4.0 && dist2 > 0.25)  // only chase if “close enough” and avoid div0
    {
        // normalize (dx,dy) and scale by ENEMY_SPEED
        double inv_dist = 1.0 / sqrt(dist2);
        double nx = dx * inv_dist;
        double ny = dy * inv_dist;

        // move the enemy
        *ex += nx * ENEMY_SPEED;
        *ey += ny * ENEMY_SPEED;
    }
	update_enemy_anim(data, *ex - prev_x, *ey - prev_y);
	data->enemy_prev_x = *ex;
	data->enemy_prev_y = *ey;
}

static double	smooth_to(double current, double target, double sharpness, double dt)
{
	double	t;

	t = 1.0 - exp(-sharpness * dt);
	return current + (target - current) * t;
}

static double	clamp_double(double v, double lo, double hi)
{
	if (v < lo)
		return (lo);
	if (v > hi)
		return (hi);
	return (v);
}

static void handle_shake(t_data *data)
{
	double	now;
	double	dt;
	double	dx;
	double	dy;
	double	speed;
	double	bob_x;
	double	bob_y;
	double	hud_bob_x;
	double	hud_bob_y;
	double	ang_delta;
	double	ang_vel;
	double	sway_x;
	double	recoil;
	double	roll_target;
	double	hud_roll_target;
	double	target_world_x;
	double	target_world_y;
	double	target_hud_x;
	double	target_hud_y;

	now = mlx_get_time();
	dt = now - data->window_time;
	if (dt < 0.0)
		dt = 0.0;
	else if (dt > 0.05)
		dt = 0.05;
	data->window_time = now;
	dx = data->player.x - data->camera.prev_px;
	dy = data->player.y - data->camera.prev_py;
	speed = 0.0;
	if (dt > 0.0)
		speed = sqrt(dx * dx + dy * dy) / dt;
	if (speed > 0.0001)
		data->camera.bob_phase += speed * VIEW_BOB_FREQ * dt;
	bob_x = sin(data->camera.bob_phase * 0.5) * VIEW_BOB_AMP_X;
	bob_y = sin(data->camera.bob_phase) * VIEW_BOB_AMP_Y;
	hud_bob_x = sin(data->camera.bob_phase * 0.5) * HUD_BOB_AMP_X;
	hud_bob_y = sin(data->camera.bob_phase) * HUD_BOB_AMP_Y;
	ang_delta = data->player.angle - data->camera.prev_angle;
	while (ang_delta > M_PI)
		ang_delta -= 2 * M_PI;
	while (ang_delta < -M_PI)
		ang_delta += 2 * M_PI;
	ang_vel = 0.0;
	if (dt > 0.0)
		ang_vel = ang_delta / dt;
	sway_x = clamp_double(ang_vel * VIEW_SWAY_SCALE, -VIEW_SWAY_MAX, VIEW_SWAY_MAX);
	roll_target = clamp_double(ang_vel * VIEW_ROLL_SCALE, -VIEW_ROLL_MAX, VIEW_ROLL_MAX);
	hud_roll_target = clamp_double(-ang_vel * HUD_ROLL_SCALE, -HUD_ROLL_MAX, HUD_ROLL_MAX);
	recoil = 0.0;
	if (data->is_player_shooting)
	{
		data->camera.recoil_phase += dt * SHAKE_VEL_RECO;
		recoil = sin(data->camera.recoil_phase) * PIX_RECOIL;
	}
	else
	{
		data->camera.recoil_phase = 0.0;
	}
	target_world_x = bob_x + sway_x;
	target_world_y = bob_y + recoil * RECOIL_WORLD_SCALE;
	target_hud_x = hud_bob_x + sway_x * 0.3;
	target_hud_y = hud_bob_y + recoil * RECOIL_HUD_SCALE;
	data->camera.world_offset_x = smooth_to(data->camera.world_offset_x,
			target_world_x, VIEW_SMOOTH, dt);
	data->camera.world_offset_y = smooth_to(data->camera.world_offset_y,
			target_world_y, VIEW_SMOOTH, dt);
	data->camera.roll = smooth_to(data->camera.roll, roll_target, VIEW_SMOOTH, dt);
	data->camera.hud_offset_x = smooth_to(data->camera.hud_offset_x,
			target_hud_x, HUD_SMOOTH, dt);
	data->camera.hud_offset_y = smooth_to(data->camera.hud_offset_y,
			target_hud_y, HUD_SMOOTH, dt);
	data->camera.hud_roll = smooth_to(data->camera.hud_roll,
			hud_roll_target, HUD_SMOOTH, dt);
	data->camera.prev_angle = data->player.angle;
	data->camera.prev_px = data->player.x;
	data->camera.prev_py = data->player.y;
}

void *ai_worker(void *arg)
{
    t_data *data = arg;
    const char *sheet_path  = "./textures/hand/hand_sheet.png";
    const char *archive_dir = "./textures/hand/ai_gen";
    struct stat st;

    // 1) Archive old sheet (with timestamp suffix if needed)
    if (access(sheet_path, F_OK) == 0 &&
        stat(archive_dir, &st) == 0 && S_ISDIR(st.st_mode))
    {
        char base_dest[PATH_MAX], dest[PATH_MAX];
        snprintf(base_dest, sizeof(base_dest), "%s/hand_sheet.png", archive_dir);

        if (access(base_dest, F_OK) == 0) {
            time_t t = time(NULL);
            snprintf(dest, sizeof(dest), "%s/hand_sheet_%ld.png", archive_dir, (long)t);
        } else {
            strcpy(dest, base_dest);
        }

        if (rename(sheet_path, dest) != 0)
            perror("Failed to archive old hand_sheet");
        else
            printf("Archived old sheet to %s\n", dest);
    }

    // 2) Ask OpenAI for a fresh 1024×1024 2×2 sheet
    // const char *prompt =
    //     "generate sprite sheet (shooting sequence) of 4 frames: pixel art, retro first-person view of a hand holding a rat, background is black, no transparency, 1990s video game style. The whole sequence will be compressed in a single sheet of 4 frames on a 2 x 2 grid. 1024x1024. there's movement in the shooting because of recoil";
    // generate_with_gpt_image(prompt, sheet_path);
	char object_name[MAX_NAME_LEN + 1];
	if (data->name_length >= 2) {
		// use the entered name
		snprintf(object_name, sizeof(object_name), "%s", data->player_name);
	} else {
		// default fallback
		strcpy(object_name, "gun");
	}

	char prompt[1024];
	snprintf(prompt, sizeof(prompt),
		"generate a sprite sheet (shooting-like sequence with visible recoil movement) of 4 frames: pixel art, retro first-person view of a hand holding a %s, %s is the actual gun and represent it from POV view perspective, you are free to imagine how the %s object would try to shoot if it were a gun, no background, 1990s video game style. The whole sequence will be compressed in a single sheet of 4 frames on a 2x2 grid",
		object_name, object_name, object_name
	);
	generate_with_gpt_image(prompt, sheet_path);

    // 3) Load it and slice into four 512×512 files
    int w, h, comp;
    unsigned char *pixels = stbi_load(sheet_path, &w, &h, &comp, 4);
    if (!pixels) {
        fprintf(stderr, "Failed to load sheet `%s`\n", sheet_path);
    }
    else if (w != 1024 || h != 1024) {
        fprintf(stderr,
                "Unexpected sheet size %dx%d (need 1024x1024)\n", w, h);
        stbi_image_free(pixels);
    }
    else {
        const int cols    = 2, rows = 2;
        const int slice_w = w  / cols;   // 512
        const int slice_h = h  / rows;   // 512

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                int idx = row * cols + col;  // 0..3
                unsigned char *slice = malloc(slice_w * slice_h * 4);
                if (!slice) continue;

                for (int yy = 0; yy < slice_h; ++yy) {
                    memcpy(
                        slice + yy * slice_w * 4,
                        pixels + ((row * slice_h + yy) * w + (col * slice_w)) * 4,
                        slice_w * 4
                    );
                }

                char outpath[PATH_MAX];
                snprintf(outpath, sizeof(outpath), "./textures/hand/handx%d.png", idx + 1);
                stbi_write_png(outpath, slice_w, slice_h,
                               4, slice, slice_w * 4);
                free(slice);
            }
        }
        stbi_image_free(pixels);
    }
    // 4) Tell main loop we’re done
    data->is_gun_ready    = true;
    // data->calling_new_gun = false;
    return NULL;
}

static void handle_new_gun(t_data *data)
{
    static double last_time = 0.0;
    bool           key2     = mlx_is_key_down(data->mlx, MLX_KEY_2);
    double         now      = mlx_get_time();

    if (key2 && now - last_time > 1.5 && !data->calling_new_gun)
	// if (key2 && now - last_time > 1.5)
    {
        last_time             = now;
        // data->calling_new_gun = true;
        // data->is_gun_ready    = false;
		// data->player_name[0] = 0;
		// data->game_state = STATE_MENU;
		// if (data->player_name[0] != 0)
		// {
		// 	printf("Gun name is %s\n", data->player_name);
		// 	printf("Spawning AI worker at %.2f\n", now);

		// 	pthread_t tid;
		// 	pthread_create(&tid, NULL, ai_worker, data);
		// 	pthread_detach(tid);
		// }
		// pause the main loop and drop into the entry‐screen
		data->name_length    = 0;
		data->player_name[0] = '\0';
		data->game_state     = STATE_MENU;
    }
}

static void	handle_shooting(t_data *data)
{
	if (mlx_is_key_down(data->mlx, MLX_KEY_SPACE))
		data->is_player_shooting = true;
	if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_LEFT))
		data->is_player_shooting = true;
}
 
static void	handle_rotation(t_data *data)
{
	if (mlx_is_key_down(data->mlx, MLX_KEY_LEFT))
		data->player.angle -= data->player.speed;
	if (mlx_is_key_down(data->mlx, MLX_KEY_RIGHT))
		data->player.angle += data->player.speed;
}

static void handle_mouse_rotation(t_data *data)
{
	int32_t aux_x = 0;
	int32_t aux_y = 0;
	int32_t dx = 0;
	//int32_t dy = 0;
	
	// if (!data->flag)
	// {
	// 	mlx_set_cursor_mode(data->mlx, MLX_MOUSE_DISABLED);
	// 	return ;
	// }
	if (mlx_is_key_down(data->mlx, MLX_KEY_TAB))
		data->camera.toggle = 0;
	if (!data->camera.toggle && mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_LEFT))
		data->camera.toggle = 1;
	if (data->camera.toggle == 0)
	{
		mlx_set_cursor_mode(data->mlx, MLX_MOUSE_NORMAL);
		return ;
	}
	mlx_get_mouse_pos(data->mlx, &aux_x, &aux_y);
	dx = aux_x - data->camera.x;
	//dy = aux_y - data->camera.y;
	if (dx)
		data->player.angle += dx * MOUSE_SENSITIVITY;
	mlx_set_cursor_mode(data->mlx, MLX_MOUSE_HIDDEN);
	data->camera.x = data->width / 2;
	data->camera.y = data->height / 2;
	mlx_set_mouse_pos(data->mlx, data->camera.x, data->camera.y);
}

void my_resize_callback(int width, int height, void* param)
{
	t_data *data = (t_data *)param;

	if (width <= MIN_WIDTH || height <= MIN_HEIGHT)
	{
		if (width == MIN_WIDTH && height == MIN_HEIGHT)
		{
			printf("Reached smallest resolution: %i x %i\n", width, height);
			return ;
		}
		if (width <= MIN_WIDTH)
			data->new_width = MIN_WIDTH;
		else
			data->new_width = width;
		if (height <= MIN_HEIGHT)
			data->new_height = MIN_HEIGHT;
		else
			data->new_height = height;
	}
	else
	{
		data->new_width = width;
		data->new_height = height;
	}
	data->resize_pending = true;
	printf("Resize Callback: new dimensions set to %i x %i\n", data->new_width, data->new_height);
}

static void	render(t_data *data)
{
	// mlx_delete_image(data->mlx, data->image);
	mlx_resize_hook(data->mlx, &my_resize_callback, (void *)data);
	// data->image = mlx_new_image(data->mlx, data->width, data->height);
	// mlx_image_to_window(data->mlx, data->image, 0, 0);
	if (data->resize_pending)
	{
		data->width = data->new_width;
		data->height = data->new_height;
		gc_free(data->zBuffer);
		//free(data->zBuffer);
		data->zBuffer = gc_alloc(sizeof(double) * data->width);
		if (!data->zBuffer)
		{
			printf("Error: Failed to allocate memory for zBuffer\n");
			return; //exit if possible
		}
		//if (data->image)
		mlx_delete_image(data->mlx, data->image);
		data->image = mlx_new_image(data->mlx, data->new_width, data->new_height);
		mlx_image_to_window(data->mlx, data->image, 0, 0);
		//if (!data->image)
		data->camera.x = data->width / 2;
		data->camera.y = data->height / 2;
		printf("Render will now happen with %dx%d\n", data->width, data->height);
		// If we’re in the menu, toss the old text so entry_screen will recalc it
		// if (data->game_state == STATE_MENU)
		// {
		// 	if (data->menu_text)
		// 	{
		// 		mlx_delete_image(data->mlx, data->menu_text);
		// 		data->menu_text = NULL;
		// 	}
		// 	if (data->input_text)
		// 	{
		// 		mlx_delete_image(data->mlx, data->input_text);
		// 		data->input_text = NULL;
		// 	}
		// }
		data->resize_pending = false;
	}
	if (data->game_state == STATE_MENU)
    {
        // Draw your entry UI instead of the 3D scene
        entry_screen(data);
        return;
    }
	draw_floor_and_ceiling(data);
	//draw_walls(data);
	//draw_sprites(data);
	draw_walls_and_sprites(data);
	draw_mini_map(data);
	draw_mini_player(data);
	draw_mini_rays(data);
	// Draw HUD hands
	draw_hud_hands(data);
    // Hud animation
	shooting_animation(data);
}

void	loop_hook(void *param)
{
	t_data	*data;
	data = (t_data *)param;
	handle_new_gun(data);
	if (data->calling_new_gun && data->is_gun_ready)
		try_load_hands(data);
	handle_movement(data);
	handle_shake(data);
	handle_shooting(data);
	handle_mouse_rotation(data);
	handle_rotation(data);
	handle_enemy(data);
	render(data);
	update_doors(data);
	if (mlx_is_key_down(data->mlx, MLX_KEY_ESCAPE))
		mlx_close_window(data->mlx);
}
