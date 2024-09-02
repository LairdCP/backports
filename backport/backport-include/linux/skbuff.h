#ifndef __BACKPORT_LINUX_SKBUFF_H
#define __BACKPORT_LINUX_SKBUFF_H
#include_next <linux/skbuff.h>
#include <linux/version.h>
#include <generated/utsrelease.h>

#if LINUX_VERSION_IS_LESS(3,4,0) && \
      (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,4)) && \
      !(defined(CONFIG_SUSE_KERNEL) && LINUX_VERSION_IS_GEQ(3,0,0))
#define skb_add_rx_frag(skb, i, page, off, size, truesize) \
	skb_add_rx_frag(skb, i, page, off, size)
#endif

#if LINUX_VERSION_IS_LESS(3,3,0)
#define __pskb_copy LINUX_BACKPORT(__pskb_copy)
extern struct sk_buff *__pskb_copy(struct sk_buff *skb,
				   int headroom, gfp_t gfp_mask);

#if !defined(SO_WIFI_STATUS)
#define skb_complete_wifi_ack LINUX_BACKPORT(skb_complete_wifi_ack)
static inline void skb_complete_wifi_ack(struct sk_buff *skb, bool acked)
{
	WARN_ON(1);
}
#endif

/* define to 0 so checks for it are always false */
#define SKBTX_WIFI_STATUS 0
#elif LINUX_VERSION_IS_LESS(3,18,0)
#define skb_complete_wifi_ack LINUX_BACKPORT(skb_complete_wifi_ack)
void skb_complete_wifi_ack(struct sk_buff *skb, bool acked);
#endif

#if LINUX_VERSION_IS_LESS(3,2,0)
#include <linux/dma-mapping.h>

/* mask skb_frag_page as RHEL6 backports this */
#define skb_frag_page LINUX_BACKPORT(skb_frag_page)
static inline struct page *skb_frag_page(const skb_frag_t *frag)
{
	return frag->page;
}

#define skb_frag_size LINUX_BACKPORT(skb_frag_size)
static inline unsigned int skb_frag_size(const skb_frag_t *frag)
{
	return frag->size;
}

/* mask skb_frag_dma_map as RHEL6 backports this */
#define skb_frag_dma_map LINUX_BACKPORT(skb_frag_dma_map)
static inline dma_addr_t skb_frag_dma_map(struct device *dev,
					  const skb_frag_t *frag,
					  size_t offset, size_t size,
					  enum dma_data_direction dir)
{
	return dma_map_page(dev, skb_frag_page(frag),
			    frag->page_offset + offset, size, dir);
}
#endif

#if LINUX_VERSION_IS_LESS(3,1,0)
/* mask __netdev_alloc_skb_ip_align as RHEL6 backports this */
#define __netdev_alloc_skb_ip_align(a,b,c) compat__netdev_alloc_skb_ip_align(a,b,c)
static inline struct sk_buff *__netdev_alloc_skb_ip_align(struct net_device *dev,
							  unsigned int length, gfp_t gfp)
{
	struct sk_buff *skb = __netdev_alloc_skb(dev, length + NET_IP_ALIGN, gfp);

	if (NET_IP_ALIGN && skb)
		skb_reserve(skb, NET_IP_ALIGN);
	return skb;
}
#endif

#if LINUX_VERSION_IS_LESS(2,6,38)
#define skb_checksum_start_offset LINUX_BACKPORT(skb_checksum_start_offset)
static inline int skb_checksum_start_offset(const struct sk_buff *skb)
{
	return skb->csum_start - skb_headroom(skb);
}
#endif

#ifndef skb_walk_frags
#define skb_walk_frags(skb, iter)	\
	for (iter = skb_shinfo(skb)->frag_list; iter; iter = iter->next)
#endif

#if LINUX_VERSION_IS_LESS(3,2,0)
#define skb_frag_size_sub LINUX_BACKPORT(skb_frag_size_sub)
static inline void skb_frag_size_sub(skb_frag_t *frag, int delta)
{
	frag->size -= delta;
}

/**
 * skb_frag_address - gets the address of the data contained in a paged fragment
 * @frag: the paged fragment buffer
 *
 * Returns the address of the data within @frag. The page must already
 * be mapped.
 */
#define skb_frag_address LINUX_BACKPORT(skb_frag_address)
static inline void *skb_frag_address(const skb_frag_t *frag)
{
	return page_address(skb_frag_page(frag)) + frag->page_offset;
}
#endif /* LINUX_VERSION_IS_LESS(3,2,0) */

#if LINUX_VERSION_IS_LESS(3,9,0)
#ifndef NETDEV_FRAG_PAGE_MAX_ORDER
#define NETDEV_FRAG_PAGE_MAX_ORDER get_order(32768)
#endif
#ifndef NETDEV_FRAG_PAGE_MAX_SIZE
#define NETDEV_FRAG_PAGE_MAX_SIZE  (PAGE_SIZE << NETDEV_FRAG_PAGE_MAX_ORDER)
#endif
#endif /* LINUX_VERSION_IS_LESS(3,9,0) */

#if LINUX_VERSION_IS_LESS(3,9,0)
#define skb_unclone LINUX_BACKPORT(skb_unclone)
static inline int skb_unclone(struct sk_buff *skb, gfp_t pri)
{
	might_sleep_if(pri & __GFP_WAIT);
	if (skb_cloned(skb))
		return pskb_expand_head(skb, 0, 0, pri);
       return 0;
}
#endif

#if LINUX_VERSION_IS_LESS(3,2,0)

#define skb_frag_address_safe LINUX_BACKPORT(skb_frag_address_safe)
/**
 * skb_frag_address_safe - gets the address of the data contained in a paged fragment
 * @frag: the paged fragment buffer
 *
 * Returns the address of the data within @frag. Checks that the page
 * is mapped and returns %NULL otherwise.
 */
static inline void *skb_frag_address_safe(const skb_frag_t *frag)
{
	void *ptr = page_address(skb_frag_page(frag));
	if (unlikely(!ptr))
		return NULL;

	return ptr + frag->page_offset;
}
#endif /* LINUX_VERSION_IS_LESS(3,2,0) */

#if LINUX_VERSION_IS_LESS(3,14,0) && \
    RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7,0) && \
    !(LINUX_VERSION_CODE == KERNEL_VERSION(3,13,11) && UTS_UBUNTU_RELEASE_ABI > 30)
/*
 * Packet hash types specify the type of hash in skb_set_hash.
 *
 * Hash types refer to the protocol layer addresses which are used to
 * construct a packet's hash. The hashes are used to differentiate or identify
 * flows of the protocol layer for the hash type. Hash types are either
 * layer-2 (L2), layer-3 (L3), or layer-4 (L4).
 *
 * Properties of hashes:
 *
 * 1) Two packets in different flows have different hash values
 * 2) Two packets in the same flow should have the same hash value
 *
 * A hash at a higher layer is considered to be more specific. A driver should
 * set the most specific hash possible.
 *
 * A driver cannot indicate a more specific hash than the layer at which a hash
 * was computed. For instance an L3 hash cannot be set as an L4 hash.
 *
 * A driver may indicate a hash level which is less specific than the
 * actual layer the hash was computed on. For instance, a hash computed
 * at L4 may be considered an L3 hash. This should only be done if the
 * driver can't unambiguously determine that the HW computed the hash at
 * the higher layer. Note that the "should" in the second property above
 * permits this.
 */
enum pkt_hash_types {
	PKT_HASH_TYPE_NONE,	/* Undefined type */
	PKT_HASH_TYPE_L2,	/* Input: src_MAC, dest_MAC */
	PKT_HASH_TYPE_L3,	/* Input: src_IP, dst_IP */
	PKT_HASH_TYPE_L4,	/* Input: src_IP, dst_IP, src_port, dst_port */
};

static inline void
skb_set_hash(struct sk_buff *skb, __u32 hash, enum pkt_hash_types type)
{
#if LINUX_VERSION_IS_GEQ(3,2,0) /* 4031ae6edb */
	skb->l4_rxhash = (type == PKT_HASH_TYPE_L4);
#endif
#if LINUX_VERSION_IS_GEQ(3,4,0) /* bdeab99191 */
	skb->rxhash = hash;
#endif
}
#endif /* LINUX_VERSION_IS_LESS(3,14,0) */

#if LINUX_VERSION_IS_LESS(3,14,0)
#define skb_get_hash(x) skb_get_rxhash(x)

#if LINUX_VERSION_IS_GEQ(3,3,0)
bool skb_needs_linearize(struct sk_buff *skb, netdev_features_t features);
#else
bool skb_needs_linearize(struct sk_buff *skb, u32 features);
#endif
#endif


#if LINUX_VERSION_IS_LESS(3,16,0)
#define __pskb_copy_fclone LINUX_BACKPORT(__pskb_copy_fclone)
static inline struct sk_buff *__pskb_copy_fclone(struct sk_buff *skb,
						 int headroom, gfp_t gfp_mask,
						 bool fclone)
{
	return __pskb_copy(skb, headroom, gfp_mask);
}
#endif

#if LINUX_VERSION_IS_LESS(3,18,0)
#define skb_clone_sk LINUX_BACKPORT(skb_clone_sk)
struct sk_buff *skb_clone_sk(struct sk_buff *skb);
#endif

#if LINUX_VERSION_IS_LESS(3,19,0)
/**
 * __dev_alloc_pages - allocate page for network Rx
 * @gfp_mask: allocation priority. Set __GFP_NOMEMALLOC if not for network Rx
 * @order: size of the allocation
 *
 * Allocate a new page.
 *
 * %NULL is returned if there is no free memory.
*/
#define __dev_alloc_pages LINUX_BACKPORT(__dev_alloc_pages)
static inline struct page *__dev_alloc_pages(gfp_t gfp_mask,
					     unsigned int order)
{
	/* This piece of code contains several assumptions.
	 * 1.  This is for device Rx, therefor a cold page is preferred.
	 * 2.  The expectation is the user wants a compound page.
	 * 3.  If requesting a order 0 page it will not be compound
	 *     due to the check to see if order has a value in prep_new_page
	 * 4.  __GFP_MEMALLOC is ignored if __GFP_NOMEMALLOC is set due to
	 *     code in gfp_to_alloc_flags that should be enforcing this.
	 */
	gfp_mask |= __GFP_COLD | __GFP_COMP;
#if LINUX_VERSION_IS_GEQ(3,6,0)
	gfp_mask |= __GFP_MEMALLOC;
#endif

	return alloc_pages_node(NUMA_NO_NODE, gfp_mask, order);
}

#define dev_alloc_pages LINUX_BACKPORT(dev_alloc_pages)
static inline struct page *dev_alloc_pages(unsigned int order)
{
	return __dev_alloc_pages(GFP_ATOMIC, order);
}

/**
 * __dev_alloc_page - allocate a page for network Rx
 * @gfp_mask: allocation priority. Set __GFP_NOMEMALLOC if not for network Rx
 *
 * Allocate a new page.
 *
 * %NULL is returned if there is no free memory.
 */
#define __dev_alloc_page LINUX_BACKPORT(__dev_alloc_page)
static inline struct page *__dev_alloc_page(gfp_t gfp_mask)
{
	return __dev_alloc_pages(gfp_mask, 0);
}

#define dev_alloc_page LINUX_BACKPORT(dev_alloc_page)
static inline struct page *dev_alloc_page(void)
{
	return __dev_alloc_page(GFP_ATOMIC);
}
#endif /* LINUX_VERSION_IS_LESS(3,19,0) */

#if LINUX_VERSION_IS_LESS(3,19,0)
#define skb_copy_datagram_msg LINUX_BACKPORT(skb_copy_datagram_msg)
static inline int skb_copy_datagram_msg(const struct sk_buff *from, int offset,
					struct msghdr *msg, int size)
{
	return skb_copy_datagram_iovec(from, offset, msg->msg_iov, size);
}

#define memcpy_from_msg LINUX_BACKPORT(memcpy_from_msg)
static inline int memcpy_from_msg(void *data, struct msghdr *msg, int len)
{
	return memcpy_fromiovec(data, msg->msg_iov, len);
}

/**
 *	skb_put_padto - increase size and pad an skbuff up to a minimal size
 *	@skb: buffer to pad
 *	@len: minimal length
 *
 *	Pads up a buffer to ensure the trailing bytes exist and are
 *	blanked. If the buffer already contains sufficient data it
 *	is untouched. Otherwise it is extended. Returns zero on
 *	success. The skb is freed on error.
 */
#define skb_put_padto LINUX_BACKPORT(skb_put_padto)
static inline int skb_put_padto(struct sk_buff *skb, unsigned int len)
{
	unsigned int size = skb->len;

	if (unlikely(size < len)) {
		len -= size;
		if (skb_pad(skb, len))
			return -ENOMEM;
		__skb_put(skb, len);
	}
	return 0;
}

#define skb_ensure_writable LINUX_BACKPORT(skb_ensure_writable)
int skb_ensure_writable(struct sk_buff *skb, int write_len);

#endif /* LINUX_VERSION_IS_LESS(3,19,0) */

#if LINUX_VERSION_IS_LESS(4,2,0)
static inline void skb_free_frag(void *data)
{
	put_page(virt_to_head_page(data));
}

#if LINUX_VERSION_IS_LESS(3,3,0)

static inline u32 skb_get_hash_perturb(struct sk_buff *skb, u32 key)
{
	return 0;
}

#else
#include <net/flow_keys.h>
#include <linux/jhash.h>

static inline u32 skb_get_hash_perturb(struct sk_buff *skb, u32 key)
{
	struct flow_keys keys;

	skb_flow_dissect(skb, &keys);
	return jhash_3words((__force u32)keys.dst,
			    (__force u32)keys.src ^ keys.ip_proto,
			    (__force u32)keys.ports, key);
}
#endif /* LINUX_VERSION_IS_LESS(3,3,0) */
#endif /* LINUX_VERSION_IS_LESS(4,2,0) */

#if LINUX_VERSION_IS_LESS(4,4,8) && \
    !LINUX_VERSION_IN_RANGE(4,1,28, 4,2,0) && \
    !LINUX_VERSION_IN_RANGE(3,16,35, 3,17,0)
#define skb_tailroom_reserve LINUX_BACKPORT(skb_tailroom_reserve)
static inline void skb_tailroom_reserve(struct sk_buff *skb, unsigned int mtu,
					unsigned int needed_tailroom)
{
#if LINUX_VERSION_IS_GEQ(3,8,5) || \
    LINUX_VERSION_IN_RANGE(3,4,38, 3,5,0) || \
    LINUX_VERSION_IN_RANGE(3,2,42, 3,3,0)
	SKB_LINEAR_ASSERT(skb);
	if (mtu < skb_tailroom(skb) - needed_tailroom)
		/* use at most mtu */
		skb->reserved_tailroom = skb_tailroom(skb) - mtu;
	else
		/* use up to all available space */
		skb->reserved_tailroom = needed_tailroom;
#else
/* older kernels have no definition for reserved tailroom */
#endif
}
#endif

#if LINUX_VERSION_IS_LESS(4,9,0)
#define __skb_set_length LINUX_BACKPORT(__skb_set_length)
static inline void __skb_set_length(struct sk_buff *skb, unsigned int len)
{
	if (unlikely(skb_is_nonlinear(skb))) {
		WARN_ON(1);
		return;
	}
	skb->len = len;
	skb_set_tail_pointer(skb, len);
}

#define __skb_grow LINUX_BACKPORT(__skb_grow)
static inline int __skb_grow(struct sk_buff *skb, unsigned int len)
{
	unsigned int diff = len - skb->len;

	if (skb_tailroom(skb) < diff) {
		int ret = pskb_expand_head(skb, 0, diff - skb_tailroom(skb),
					   GFP_ATOMIC);
		if (ret)
			return ret;
	}
	__skb_set_length(skb, len);
	return 0;
}
#endif

#if LINUX_VERSION_IS_LESS(4,13,0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7,6)
static inline void *backport_skb_put(struct sk_buff *skb, unsigned int len)
{
	return skb_put(skb, len);
}
#define skb_put LINUX_BACKPORT(skb_put)

static inline void *backport_skb_push(struct sk_buff *skb, unsigned int len)
{
	return skb_push(skb, len);
}
#define skb_push LINUX_BACKPORT(skb_push)

static inline void *backport___skb_push(struct sk_buff *skb, unsigned int len)
{
	return __skb_push(skb, len);
}
#define __skb_push LINUX_BACKPORT(__skb_push)

static inline void *__skb_put_zero(struct sk_buff *skb, unsigned int len)
{
	void *tmp = __skb_put(skb, len);

	memset(tmp, 0, len);
	return tmp;
}

static inline void *skb_put_zero(struct sk_buff *skb, unsigned int len)
{
	void *tmp = skb_put(skb, len);

	memset(tmp, 0, len);

	return tmp;
}

static inline void *skb_put_data(struct sk_buff *skb, const void *data,
				 unsigned int len)
{
	void *tmp = skb_put(skb, len);

	memcpy(tmp, data, len);

	return tmp;
}
#define __skb_put_data skb_put_data

static inline void skb_put_u8(struct sk_buff *skb, u8 val)
{
	*(u8 *)skb_put(skb, 1) = val;
}
#endif /* LINUX_VERSION_IS_LESS(4,13,0) */

#if LINUX_VERSION_IS_LESS(4,20,0)
static inline struct sk_buff *__skb_peek(const struct sk_buff_head *list_)
{
	return list_->next;
}

#if !LINUX_VERSION_IN_RANGE(4,19,10, 4,20,0) && \
    !LINUX_VERSION_IN_RANGE(4,14,217, 4,15,0)
static inline void skb_mark_not_on_list(struct sk_buff *skb)
{
	skb->next = NULL;
}
#endif /* 4.19.10 <= x < 4.20 */
#endif

#if LINUX_VERSION_IS_LESS(3,4,0)
static inline struct sk_buff *skb_peek_next(struct sk_buff *skb,
		const struct sk_buff_head *list_)
{
	struct sk_buff *next = skb->next;
	if (next == (struct sk_buff *)list_)
		next = NULL;
	return next;
}
#endif

#if (LINUX_VERSION_IN_RANGE(3,3,0, 3,10,0) || LINUX_VERSION_IS_LESS(3,2,98))
void kfree_skb_list(struct sk_buff *segs);
#endif

#if LINUX_VERSION_IS_LESS(4,11,0)
#define skb_mac_offset LINUX_BACKPORT(skb_mac_offset)
static inline int skb_mac_offset(const struct sk_buff *skb)
{
	return skb_mac_header(skb) - skb->data;
}
#endif

#if LINUX_VERSION_IS_LESS(5,4,0)
/**
 * skb_frag_off() - Returns the offset of a skb fragment
 * @frag: the paged fragment
 */
#define skb_frag_off LINUX_BACKPORT(skb_frag_off)
static inline unsigned int skb_frag_off(const skb_frag_t *frag)
{
	return frag->page_offset;
}

#define nf_reset_ct LINUX_BACKPORT(nf_reset_ct)
static inline void nf_reset_ct(struct sk_buff *skb)
{
	nf_reset(skb);
}

#define skb_queue_empty_lockless LINUX_BACKPORT(skb_queue_empty_lockless)
static inline bool skb_queue_empty_lockless(const struct sk_buff_head *list)
{
	return READ_ONCE(list->next) == (const struct sk_buff *) list;
}
#endif

#ifndef skb_list_walk_safe
#define skb_list_walk_safe(first, skb, next_skb)				\
	for ((skb) = (first), (next_skb) = (skb) ? (skb)->next : NULL; (skb); 	\
	     (skb) = (next_skb), (next_skb) = (skb) ? (skb)->next : NULL)
#endif

#if LINUX_VERSION_IS_LESS(5,6,0) &&			\
	!LINUX_VERSION_IN_RANGE(5,4,69, 5,5,0) &&	\
	!LINUX_VERSION_IN_RANGE(4,19,149, 4,20,0) &&	\
	!LINUX_VERSION_IN_RANGE(4,14,200, 4,15,0) &&	\
	!LINUX_VERSION_IN_RANGE(4,9,238, 4,10,0) &&	\
	!LINUX_VERSION_IN_RANGE(4,4,238, 4,5,0)
/**
 *	skb_queue_len_lockless	- get queue length
 *	@list_: list to measure
 *
 *	Return the length of an &sk_buff queue.
 *	This variant can be used in lockless contexts.
 */
#define skb_queue_len_lockless LINUX_BACKPORT(skb_queue_len_lockless)
static inline __u32 skb_queue_len_lockless(const struct sk_buff_head *list_)
{
	return READ_ONCE(list_->qlen);
}
#endif /* < 5.6.0 */

#if LINUX_VERSION_IS_LESS(5,10,54)
/* Coverage collection */
#define skb_set_kcov_handle LINUX_BACKPORT(skb_set_kcov_handle)
static inline void skb_set_kcov_handle(struct sk_buff *skb,
                                       const u64 kcov_handle) { }
#define skb_get_kcov_handle LINUX_BACKPORT(skb_get_kcov_handle)
static inline u64 skb_get_kcov_handle(struct sk_buff *skb) { return 0; }
#endif

#if LINUX_VERSION_IS_LESS(5,17,0)
static inline void *skb_pull_data(struct sk_buff *skb, size_t len)
{
	void *data = skb->data;

	if (skb->len < len)
		return NULL;

	skb_pull(skb, len);

	return data;
}
#endif /* < 5.17.0 */

#if LINUX_VERSION_IS_LESS(5,17,0) && \
	!LINUX_VERSION_IN_RANGE(5,15,58, 5,16,0)
#define kfree_skb_reason(skb, reason) kfree_skb(skb)
#endif	/* < 5.17.0 */

#if LINUX_VERSION_IS_LESS(5,18,0)
static inline struct sk_buff *backport_skb_recv_datagram(struct sock *sk, unsigned int flags, int *err)
{
	return skb_recv_datagram(sk, flags, (flags & MSG_DONTWAIT) ? 1 : 0, err);
}
#define skb_recv_datagram LINUX_BACKPORT(skb_recv_datagram)
#endif /* < 5.18.0 */

#if LINUX_VERSION_IS_LESS(6,4,0)
#ifndef CONFIG_PAGE_POOL
static inline void skb_mark_for_recycle(struct sk_buff *skb)
{
}
#endif /* CONFIG_PAGE_POOL */
#endif /* < 6.4.0 */

#endif /* __BACKPORT_LINUX_SKBUFF_H */
