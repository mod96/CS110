The shell will keep a list of all background processes, and it will have some standard shell abilities:
* you can quit the shell (using quit or exit)
* you can bring them to the front (using  fg)
* you can continue a background job (using bg)
* you can kill a set of processes in a pipeline (using slay) (this will entail learning about process groups)
* you can stop a process (using halt)
* you can continue a process (using cont)
* you can get a list of jobs (using jobs)
* you are responsible for creating pipelines that enable you to send output between programs:
  * ls | grep stsh | cut -d- -f2
  * sort < stsh.cc | wc > stsh-wc.txt
* You will also be handing off terminal control to foreground processes, which is new
* You will also need to use a handler to capture SIGTSTP and  SIGINT to capture ctrl-Z and ctrl-C, respectively (notice that these don't affect your regular shell -- they shouldn't affect your shell, either).

Be aware:

* You will only need to modify stsh.cc
* You can test your shell programmatically with samples/stsh-driver
* One of the more difficult parts of the assignment is making sure you are keeping track of all the processes you've launched correctly.
* There is a very good list of milestones in the assignment -- try to accomplish regular milestones, and you should stay on track. (don't skip any milestone!)