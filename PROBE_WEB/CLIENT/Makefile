# Makefile for generation of jar archive from probe client classes

jar:
	javac *.java
	jar -cmf mymanifest allProbes.jar *.class  
##demo.newick

update:
	jar -uf allProbes.jar *.class

test:
	( mkdir -p testdir; cd testdir; java -jar ../allProbes.jar; ls -al -rt )

clean:
	rm *.class
	rm allProbes.jar
