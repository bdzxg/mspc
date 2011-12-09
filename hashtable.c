#include "proxy.h"

int ht_set(ht_table_t* t,ht_key_t k,void* data){return -1;}
void* ht_get(ht_table_t* t,ht_key_t k){ return NULL; }
void ht_remove(ht_table_t* t,ht_key_t k){}


int 
ht_resize_tmp(ht_table_t* t)
{
  int len,pos;
  uint32_t hash;
  ht_node_t *node,*n,*nnlast;

  len = t->len;
  len = len << 1;

  if(len<0) {
    L("Error, Max Length");
    return -1;
  }

  
  //ht_node_t* nn = (ht_node_t*)calloc((size_t)len,sizeof(ht_node_t));
  ht_node_t* nn = (ht_node_t*)pxy_calloc(len*sizeof(ht_node_t));

  /*recalcute the hash*/
  for(node=t->nodes;t;t++){
    if(node){

      hash = node->key.hash;
      pos = hash % len;
      
      ht_node_from(node,&(nn[pos]));
      
      while(node->next){
	
	n = node->next;
	nnlast = (ht_node_t*)calloc(1,sizeof(ht_node_t));
	ht_node_from(n,nnlast);

	nnlast = nnlast->next;
	node=node->next;
      }
    }
  }
  return 1;
}
