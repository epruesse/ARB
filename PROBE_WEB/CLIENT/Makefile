# Makefile for generation of jar archive from probe client classes

jar:
	javac *.java
	jar -cmf mymanifest allProbes.jar *.class demo.newick

update:
	jar -uf allProbes.jar *.class

test:
	java -jar allProbes.jar

clean:
	rm *.class
	rm allProbes.jar