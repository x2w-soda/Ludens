return {
	test_name = "Camera2D Properties",
	steps = {
		{
			scene_command = "load_scene",
			subtree = {
				{
					parent = -1,
					name = "cam",
					type = "Camera2DComponent",
					transform = {},
				},
			},
		},
		{
			scene_command = "set_props",
			path = "cam",
			props = {
				extent = {100, 200},
				zoom = 2.0
			}
		},
		{
			scene_command = "get_props",
			path = "cam",
			props = {
				extent = {100, 200},
				zoom = 2.0
			}
		},
	},
}