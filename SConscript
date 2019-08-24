target = 'w7tbp'

files = Split("""
	w7tbp.cpp
""")

resources = Split("""
	Resource.rc
""")

libs = Split("""
	kernel32
	ole32
	user32
""")

examples = ''

docs = Split("""
	w7tbp.Readme.txt
""")

Import('BuildPlugin')

BuildPlugin(target, files, libs, examples, docs, res = resources)