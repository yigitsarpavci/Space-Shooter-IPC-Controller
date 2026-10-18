# ==============================================================================
# Space Shooter: Asynchronous IPC Subsystem & Autonomous Controller Makefile
# ==============================================================================

PROJECT_DIR := 2D_SPACE_SHOOTER
STUDENT_ID  ?= 2023400048

.PHONY: all fifo keyboard clean run run-keyboard pack help

all: fifo

fifo:
	$(MAKE) -C $(PROJECT_DIR) fifo

keyboard:
	$(MAKE) -C $(PROJECT_DIR) keyboard

run:
	cd $(PROJECT_DIR) && ./run.sh $(STUDENT_ID) fifo 1 ctl

run-keyboard:
	cd $(PROJECT_DIR) && ./run.sh $(STUDENT_ID) keyboard 1

clean:
	$(MAKE) -C $(PROJECT_DIR) clean
	rm -f $(PROJECT_DIR)/implementation/game.fifo

pack: clean
	cd $(PROJECT_DIR) && zip -r ../$(STUDENT_ID).zip implementation/

help:
	@echo "Space Shooter Build & Simulation Management"
	@echo "--------------------------------------------------------"
	@echo "  make / make fifo     - Build game with student IPC & FIFO subsystem"
	@echo "  make keyboard        - Build game with keyboard input source"
	@echo "  make run             - Run autonomous simulation with winning ctl.sh"
	@echo "  make run-keyboard    - Run interactive keyboard session"
	@echo "  make pack            - Re-package implementation into submission zip"
	@echo "  make clean           - Remove all build and runtime artifacts"
