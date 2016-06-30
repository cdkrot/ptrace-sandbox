//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Sayutin Dmitry, Alferov Vasiliy
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

/** Splay tree is chosen for use in sandboxer because it has heuristically better performance on frequent 
  * queries. This realization is made very simple to avoid bugs. All standart functions should be written 
  * in each module that uses splay. It increases complexity of code but makes splay trees much more flexible.
  *
  * Some examples of working and _tested_ code can be found in splay_test module.
  *
  * Important: this version is not thread-safety, you should write your own locks in your structires.
  */


#ifndef SANDBOXER_SPLAY_H_
#define SANDBOXER_SPLAY_H_

struct splay_tree_node {
    struct splay_tree_node *L, *R, *par;
};

/** 
  * This function rebuilds the tree in some way. After its execution vertice v is the root of the tree. It 
  * can be proved that the amortized complexity of this operation is O(log n), where n is size of the tree.
  * It is strongly recomended to call this function after every search in tree,
  * The return value is v.
  */
struct splay_tree_node *splay(struct splay_tree_node *v);

#endif
