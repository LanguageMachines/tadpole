#! /bin/csh -f
python=python
rm -rf tadpole.ana.inst.*
$python scripts/pairs.py -m20 --feats-bigram  tadpole.ana > tadpole.ana.inst
$python scripts/dir.py tadpole.ana > tadpole.ana.dir.inst
$python scripts/rels.py tadpole.ana > tadpole.ana.rels.inst
