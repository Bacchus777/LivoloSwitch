const zigbeeHerdsmanConverters = require('zigbee-herdsman-converters');

const exposes = zigbeeHerdsmanConverters.exposes;
const ea = exposes.access;
const e = exposes.presets;
const fz = zigbeeHerdsmanConverters.fromZigbeeConverters;
const tz = zigbeeHerdsmanConverters.toZigbeeConverters;
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const extend = require('zigbee-herdsman-converters/lib/extend');

const bind = async (endpoint, target, clusters) => {
    for (const cluster of clusters) {
        await endpoint.bind(cluster, target);
    }
};


const device = {
	zigbeeModel: ['Livolo_2_Channel'],
	model: 'Livolo_2_Channel',
	vendor: 'Bacchus',
    description: 'Zigbee updated Livolo switch two channels',
	supports: 'on/off', 
	fromZigbee: [fz.identify, fz.on_off],
	toZigbee: [tz.on_off],
	meta: {
		multiEndpoint: true,
	},
    configure: async (device, coordinatorEndpoint) => {
        const firstEndpoint = device.getEndpoint(1);
        await reporting.bind(firstEndpoint, coordinatorEndpoint, ['genBasic', 'genIdentify', 'genOnOff']);
        await reporting.onOff(firstEndpoint);
        const secondEndpoint = device.getEndpoint(2);
        await reporting.bind(secondEndpoint, coordinatorEndpoint, ['genBasic', 'genIdentify', 'genOnOff']);
        await reporting.onOff(secondEndpoint);
    },
	endpoint: (device) => {
		return {'l1': 1, 'l2': 2}
		},
	exposes: [
			e.switch().withEndpoint('l1'),
			e.switch().withEndpoint('l2'),
			],
};

module.exports = device;

