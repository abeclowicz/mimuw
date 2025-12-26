# Macros.
BUILD_DIR = .
MODULES_DIR = .
MAIN_FILE = text_moths

# Compiler and compilation flags.
CXX = clang++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra \
           -Wno-experimental-header-units \
           -Wno-pragma-system-header-outside-header \
           -Wno-ambiguous-ellipsis \
           -fprebuilt-module-path=$(BUILD_DIR)

# Standard library headers and dependencies for each module.
HEADERS_colony = cstddef format stdexcept string unordered_map utility
DEPS_colony = colony-moth colony-text

HEADERS_command = cstddef format iostream regex stdexcept string utility
DEPS_command = colony colony-moth

HEADERS_colony-moth = algorithm cctype concepts cstddef format functional \
					  memory ostream stdexcept string unordered_map utility
DEPS_colony-moth =

HEADERS_reader = algorithm cstddef concepts format functional ios iostream \
				 memory stdexcept string unordered_map
DEPS_reader = colony-text

HEADERS_colony-text = cstddef list memory ostream stdexcept string utility
DEPS_colony-text = colony-moth

HEADERS_$(MAIN_FILE) =
DEPS_$(MAIN_FILE) = reader

USER_MODULES = colony command colony-moth reader colony-text
STD_HEADERS = $(sort $(HEADERS_colony) $(HEADERS_command) \
			  $(HEADERS_colony-moth) $(HEADERS_reader) $(HEADERS_colony-text) \
			  $(HEADERS_text_moths))

OBJ_FILES = $(USER_MODULES:%=$(BUILD_DIR)/%.o) $(BUILD_DIR)/$(MAIN_FILE).o

# Macros
define MODULE_DEPENDENCIES
$(BUILD_DIR)/$(1).pcm: $(DEPS_$(1):%=$(BUILD_DIR)/%.pcm) \
                       $(HEADERS_$(1):%=$(BUILD_DIR)/%.pch)
endef

get_pch_flags = $(addprefix -fmodule-file=$(BUILD_DIR)/, $1)

.PHONY: all clean

all: $(BUILD_DIR)/$(MAIN_FILE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(foreach mod,$(USER_MODULES) main,$(eval $(call MODULE_DEPENDENCIES,$(mod))))

$(BUILD_DIR)/%.pch: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -xc++-system-header --precompile $* -o $@

$(BUILD_DIR)/%.pcm: %.cppm | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) \
		$(call get_pch_flags, $(HEADERS_$*:%=%.pch)) \
		--precompile $< -o $@

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.pcm
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(MAIN_FILE).o: $(MAIN_FILE).cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(MAIN_FILE): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(OBJ_FILES) $(USER_MODULES:%=$(BUILD_DIR)/%.pcm) \
	      $(STD_HEADERS:%=$(BUILD_DIR)/%.pch) $(BUILD_DIR)/$(MAIN_FILE)
