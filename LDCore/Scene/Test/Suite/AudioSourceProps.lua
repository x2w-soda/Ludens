return {
	test_name = "AudioSource Properties",
	steps = {
		{
			scene_command = "load_scene",
			subtree = {
				{
					parent = -1,
					name = "source",
					type = "AudioSourceComponent",
					transform = {},
				},
			},
		},
		{
			scene_command = "set_props",
			path = "source",
			props = {
				["pan"] = -0.2,
				["volume_linear"] = -0.3,
			}
		},
		{
			scene_command = "get_props",
			path = "source",
			props = {
				["pan"] = 0.0, -- clamped
				["volume_linear"] = 0.0, -- clamped
			}
		},
		{
			scene_command = "set_props",
			path = "source",
			props = {
				["pan"] = 1.2,
				["volume_linear"] = 1.3,
			}
		},
		{
			scene_command = "get_props",
			path = "source",
			props = {
				["pan"] = 1.0, -- clamped
				["volume_linear"] = 1.0, -- clamped
			}
		},
	},
}