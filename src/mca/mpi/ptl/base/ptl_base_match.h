/*
 * $HEADER$
 */

#ifndef MCA_PTL_BASE_MATCH_H
#define MCA_PTL_BASE_MATCH_H

int mca_ptl_base_match(mca_pml_base_reliable_hdr_t *frag_header,
        mca_pml_base_recv_frag_t *frag_desc, int *match_made,
        lam_list_t *additional_matches);

mca_pml_base_recv_request_t *mca_ptl_base_check_recieves_for_match(
        mca_pml_base_reliable_hdr_t *frag_header,
        mca_pml_comm_t *pml_comm);

mca_pml_base_recv_request_t *check_wild_receives_for_match(
        mca_pml_base_reliable_hdr_t *frag_header, 
        mca_pml_comm_t *pml_comm);

mca_pml_base_recv_request_t *check_specific_receives_for_match(
        mca_pml_base_reliable_hdr_t *frag_header, 
        mca_pml_comm_t *pml_comm);

mca_pml_base_recv_request_t *check_specific_and_wild_receives_for_match(
        mca_pml_base_reliable_hdr_t *frag_header, 
        mca_pml_comm_t *pml_comm);

#endif /* MCA_PTL_BASE_MATCH_H */

