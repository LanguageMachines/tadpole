#! /bin/csh -f
python=python
rm -rf tadpole.ana.inst.*
$python pairs.py -m20 --feats-bigram  tadpole.ana > tadpole.ana.inst
$python dir.py tadpole.ana > tadpole.ana.dir.inst
$python rels.py tadpole.ana > tadpole.ana.rels.inst
