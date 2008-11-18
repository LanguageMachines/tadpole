#! /bin/csh -f
python=python
$python scripts/csidp.py -m20 --dep tadpole.ana.inst.out --mod tadpole.ana.rels.out --dir tadpole.ana.dir.out tadpole.ana >> tadpole.ana.result

