const Datastore = require('@google-cloud/datastore');
/**
 * Triggered from a message on a Cloud Pub/Sub topic.
 *
 * @param {!Object} event The Cloud Functions event.
 * @param {!Function} The callback function.
 */
exports.subscribe = function subscribe(event, callback) {
  const pubsubMessage = event.data;
  const datastore = Datastore({
    projectId: 'johansiot-199910'
  });
  const soildata = JSON.parse(Buffer.from(pubsubMessage.data, 'base64').toString());
  soildata.timestamp = new Date(pubsubMessage.attributes.published_at);

  const entity = {
    key: datastore.key(['soildata', pubsubMessage.messageId]),
    data: soildata
    };
  datastore.save(entity)
  	//.then(() => { console.log(entity) })
    .catch((err) => {
      console.error('ERROR:', err);
    });

  callback();
};
