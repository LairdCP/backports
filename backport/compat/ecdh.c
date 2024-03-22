/* ECDH key-agreement protocol
 *
 * Copyright (c) 2016, Intel Corporation
 * Authors: Salvator Benedetto <salvatore.benedetto@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <crypto/internal/kpp.h>
#include <crypto/kpp.h>
#include <crypto/ecdh.h>
#include <linux/scatterlist.h>

#if LINUX_VERSION_IS_LESS(4,8,0)
#include "ecc.h"

extern void crypto_unregister_kpp(struct kpp_alg *alg);
extern int crypto_register_kpp(struct kpp_alg *alg);

struct ecdh_ctx {
	unsigned int curve_id;
	unsigned int ndigits;
	u64 private_key[ECC_MAX_DIGITS];
};

static inline struct ecdh_ctx *ecdh_get_ctx(struct crypto_kpp *tfm)
{
	return kpp_tfm_ctx(tfm);
}

static int ecdh_set_secret(struct crypto_kpp *tfm, const void *buf,
			   unsigned int len)
{
	struct ecdh_ctx *ctx = ecdh_get_ctx(tfm);
	struct ecdh params;

	if (crypto_ecdh_decode_key(buf, len, &params) < 0 ||
	    params.key_size > sizeof(u64) * ctx->ndigits)
		return -EINVAL;

	if (!params.key || !params.key_size)
		return ecc_gen_privkey(ctx->curve_id, ctx->ndigits,
				       ctx->private_key);

	memcpy(ctx->private_key, params.key, params.key_size);

	if (ecc_is_key_valid(ctx->curve_id, ctx->ndigits,
			     ctx->private_key, params.key_size) < 0) {
		memzero_explicit(ctx->private_key, params.key_size);
		return -EINVAL;
	}
	return 0;
}

static int ecdh_compute_value(struct kpp_request *req)
{
	struct crypto_kpp *tfm = crypto_kpp_reqtfm(req);
	struct ecdh_ctx *ctx = ecdh_get_ctx(tfm);
	u64 *public_key;
	u64 *shared_secret = NULL;
	void *buf;
	size_t copied, nbytes, public_key_sz;
	int ret = -ENOMEM;

	nbytes = ctx->ndigits << ECC_DIGITS_TO_BYTES_SHIFT;
	/* Public part is a point thus it has both coordinates */
	public_key_sz = 2 * nbytes;

	public_key = kmalloc(public_key_sz, GFP_KERNEL);
	if (!public_key)
		return -ENOMEM;

	if (req->src) {
		shared_secret = kmalloc(nbytes, GFP_KERNEL);
		if (!shared_secret)
			goto free_pubkey;

		/* from here on it's invalid parameters */
		ret = -EINVAL;

		/* must have exactly two points to be on the curve */
		if (public_key_sz != req->src_len)
			goto free_all;

		copied = sg_copy_to_buffer(req->src,
					   sg_nents_for_len(req->src,
							    public_key_sz),
					   public_key, public_key_sz);
		if (copied != public_key_sz)
			goto free_all;

		ret = crypto_ecdh_shared_secret(ctx->curve_id, ctx->ndigits,
						ctx->private_key, public_key,
						shared_secret);

		buf = shared_secret;
	} else {
		ret = ecc_make_pub_key(ctx->curve_id, ctx->ndigits,
				       ctx->private_key, public_key);
		buf = public_key;
		nbytes = public_key_sz;
	}

	if (ret < 0)
		goto free_all;

	/* might want less than we've got */
	nbytes = min_t(size_t, nbytes, req->dst_len);
	copied = sg_copy_from_buffer(req->dst, sg_nents_for_len(req->dst,
								nbytes),
				     buf, nbytes);
	if (copied != nbytes)
		ret = -EINVAL;

	/* fall through */
free_all:
	kfree_sensitive(shared_secret);
free_pubkey:
	kfree(public_key);
	return ret;
}

static unsigned int ecdh_max_size(struct crypto_kpp *tfm)
{
	struct ecdh_ctx *ctx = ecdh_get_ctx(tfm);

	/* Public key is made of two coordinates, add one to the left shift */
	return ctx->ndigits << (ECC_DIGITS_TO_BYTES_SHIFT + 1);
}

static struct kpp_alg ecdh = {
	.set_secret = ecdh_set_secret,
	.generate_public_key = ecdh_compute_value,
	.compute_shared_secret = ecdh_compute_value,
	.max_size = ecdh_max_size,
	.base = {
		.cra_name = "ecdh",
		.cra_driver_name = "ecdh-generic",
		.cra_priority = 100,
		.cra_module = THIS_MODULE,
		.cra_ctxsize = sizeof(struct ecdh_ctx),
	},
};

int ecdh_init(void)
{
	return crypto_register_kpp(&ecdh);
}

void ecdh_exit(void)
{
	crypto_unregister_kpp(&ecdh);
}

#endif
