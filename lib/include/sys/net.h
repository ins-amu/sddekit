/* copyright 2016 Apache 2 libtvb authors */

#include "../libtvb.h"

/**
 * Network interface.
 *
 * A network adapts existing system(s) into a network with a connectivity,
 * while also providing a system interface, so that it can be used with a scheme
 * like any other system.
 *
 * The network combines a connectivity with one or more system definitions. For
 * each node in the network, the mapping to/from connectivity entries is determined
 * by the number of coupling & observable terms of the subsystem for each node.
 *
 * For example, a network of 100 nodes, each having a sys with ndc=2 & nobs=3, would
 * have a connectivity size 200 x 300.
 *
 */
struct tvb_net
{
	tvb_declare_common_members(tvb_net);

	/*! Get system interface for this network. */
	struct tvb_sys * (*as_sys)(struct tvb_net *net);

	/*! Get connectivity for this network. */
	struct tvb_conn * (*get_conn)(struct tvb_net *net);

	/*! Get number of nodes. */
	uint32_t (*get_n_node)(struct tvb_net *net);

	/*! Get number of subsystems. */
	uint32_t (*get_n_subsys)(struct tvb_net *net);

	/*! Get i'th subsystem. */
	struct tvb_sys * (*get_subsys)(struct tvb_net *net, uint32_t i);

	/*! Set i'th subsystem. */
	enum tvb_stat (*set_subsys)(struct tvb_net *net,
			uint32_t i_sys, struct tvb_sys * sys);

	/*! Get subsystem index of i'th node. */
	uint32_t (*get_node_subsys)(struct tvb_net *net, uint32_t i_node);

	/*! Set subsystem index of i'th node. */
	enum tvb_stat (*set_node_subsys)(struct tvb_net *net,
			uint32_t i_node, uint32_t i_sys);
};

/* TODO subsys list helper for bindings? */

/*!  Construct a new network object. */
TVB_API struct tvb_net *
tvb_net_new(uint32_t n_node, uint32_t n_subsys, uint32_t *node_subsys_map, 
	   struct tvb_sys **subsys,
	   struct tvb_conn *conn);