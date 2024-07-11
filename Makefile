MKFILE_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CC=gcc

OUTPUT_DIR=$(MKFILE_DIR)/out
CFLAGS=-g -Wall -I.

.PHONY: help

## Self help
help:
		@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

output-dir: ## Makes output directory
	@mkdir -p $(OUTPUT_DIR)

%.o: %.c output-dir
		@$(CC) -Wall -c -o $(OUTPUT_DIR)/$(notdir $(@)) $< $(CFLAGS)

## you need to generate a file, of any size called data.file. this is the file we load
## into the memory of this demo
controller: output-dir ## Builds shared mem controller
		@$(CC) $(CFLAGS) memcontroller.c -o $(OUTPUT_DIR)/controller
		@echo "built $(OUTPUT_DIR)/controller"

consumer: output-dir ## Builds shared mem consumer
		@$(CC) $(CFLAGS) memconsumer.c -o $(OUTPUT_DIR)/consumer
		@echo "built $(OUTPUT_DIR)/consumer"

## NOTE THE CONTAINER TAG
controller-image: controller ## Builds controller container image
	 sudo docker build -t khenidak/memctrl -f $(MKFILE_DIR)/controller_Dockerfile $(MKFILE_DIR)/

## NOTE THE CONTAINER TAG
consumer-image: consumer ## Builds consumer container image
	 sudo docker build -t khenidak/memconsumer -f $(MKFILE_DIR)/consumer_Dockerfile $(MKFILE_DIR)/

container-images: controller-image consumer-image ## Builds all container images
