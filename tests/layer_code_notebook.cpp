/*本网络会设计成树状或网状网络，此时将会是
 * non-beacon的 ，所以将不会有GTS传输，同时数据
 *在coord和device之间流通的时候也将是p2p的 直接发送
 */

/* Nwk layer send data to mac */
mcps_data_request()
/* Mac layer confirms nwk layer data_request result */
mcps_data_confirm()
/* Receive ata from other node will trigger indication */
mcps_data_indication()

mcps_purge_request()
mcps_purge_confirm()

mlme_associate_request()
mlme_associate_indication()
mlme_associate_response()
mlme_associate_confirm()

mlme_disassociate_request()
mlme_disassociate_indication()
mlme_disassociate_confirm()

mlme_get_request()
mlme_get_confirm()

mlme_set_request()
mlme_set_confirm()

mlme_reset_request()
mlme_reset_confirm()

mlme_rx_enable_request()
mlme_rx_enable_confirm()

mlme_scan_request()
mlme_scan_confirm()

mlme_comm_status_indication()

mlme_start_request()
mlme_start_confirm()

//mlme_beacon_notify_indication()

/* GTS not used */
//mlme_gts_request()
//mlme_gts_confirm()
//mlme_gts_indication()

/* Not use orphan feature */
//mlme_orphan_indicate()
//mlme_orphan_response()

/* non-beacon network doesnt need sync */
//mlme_sync_request()
//mlme_sync_loss_indication()

/* poll 是device主动向coord进行接收数据请求
 * 如果当前coord有数据  那么就会返回有数据的ack
 * 给poll_confirm，并且还会把数据通过下一帧发送给device
 *（触发mcps_data_indication ）;如果当前没有数据，
 * poll_confirm会返回无数据的状态。 
 */
/* non-beacon网络 不使用poll */
//mlme_poll_request()
//mlme_poll_confirm()



