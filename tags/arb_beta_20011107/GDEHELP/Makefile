all:
	$(PP)  -IMENUS ARB_GDEmenus.source >ARB_GDEmenus

save:
	rm -f arbgdemenus*.tar.gz
	tar cvf - MENUS ARB_GDEmenus* |gzip >`date '+arbgdemnus_%d_%m_%y.tar.gz'`
