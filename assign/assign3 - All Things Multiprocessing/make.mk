pipeline_src = pipeline.c pipeline-test.c 
subprocess_src = subprocess.cc subprocess-test.cc 

all: pipeline subprocess

pipeline:
	gcc $(pipeline_src) -lstdc++ -o pipeline-test 

subprocess:
	gcc $(subprocess_src) -lstdc++ -o subprocess-test 