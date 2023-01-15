pipeline_src = pipeline.c pipeline-test.c 
subprocess_src = subprocess.cc subprocess-test.cc 
farm_src = farm.cc subprocess.cc

all: pipeline subprocess farm

pipeline:
	gcc $(pipeline_src) -lstdc++ -o pipeline-test 

subprocess:
	gcc $(subprocess_src) -lstdc++ -o subprocess-test 

farm:
	gcc $(farm_src) -lstdc++ -o farm