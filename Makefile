BUILD_DIR = build
include $(N64_INST)/include/n64.mk

src = src/main.c

assets_png = $(wildcard assets/png/*.png)
assets_wav = $(wildcard assets/wav/*.wav)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64)))

MKSPRITE_FLAGS ?=
AUDIOCONV_FLAGS ?=

all: runevalley.z64

filesystem/%.sprite: assets/png/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/%.wav64: assets/wav/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) $(AUDIOCONV_FLAGS) -o filesystem "$<"

filesystem/music.wav64: AUDIOCONV_FLAGS += --wav-loop true

$(BUILD_DIR)/runevalley.dfs: $(assets_conv)
$(BUILD_DIR)/runevalley.elf: $(src:%.c=$(BUILD_DIR)/%.o)

runevalley.z64: N64_ROM_TITLE = "Rune Valley 64"
runevalley.z64: $(BUILD_DIR)/runevalley.dfs

clean:
	rm -rf $(BUILD_DIR) filesystem runevalley.z64

-include $(wildcard $(BUILD_DIR)/src/*.d)

.PHONY: all clean
