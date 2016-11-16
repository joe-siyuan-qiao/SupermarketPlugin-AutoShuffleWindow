# This script automatically generate Whitelist.json
# See the script to know how to use this

from copy import deepcopy
import json

# Your code start here

dstjsonname = 'Whitelist.json'

shelves = []
shelves.append({
	'Name': 'BP_ShelfMain',
	'Repeat': 2,
	'Scale': 1.0,
	'Shelfbase': [
		0.09464480874316935,
		0.31502732240437166,
		0.51994535519125690,
		0.72759562841530050,
		0.8938797814207660
	],
	'Shelfoffset': [
		0.035,
		0.02,
		0.02,
		0.02,
		0.02
	]
})

productgroups = []
productgroups.append({
	'GroupName': 'chips_barbaras',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 30,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_box_lays',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 30,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_box_olddutch',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 42,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_can_lays',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 40,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_cretos',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 30,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_cyclone',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 66,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_doritos',
	'ShelfName': 'BP_ShelfMain_001',
	'Repeat': 141,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_kettle',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 176,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_lays',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 234,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_sun',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 234,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_raffles',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 234,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_pringles_small',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 500,
	'Scale': 1.0,
	'Discard': True
})

productgroups.append({
	'GroupName': 'chips_pringles',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 500,
	'Scale': 1.0,
	'Discard': False
})

productgroups.append({
	'GroupName': 'chips_olddutch',
	'ShelfName': 'BP_ShelfMain_002',
	'Repeat': 1000,
	'Scale': 1.0,
	'Discard': False
})


# Your code end here

JsonRoot = {}
JsonRoot['Whitelist'] = {}

# build Shelves
JsonShelves = []
for shelf in shelves:
	for cnt in range(shelf['Repeat']):
		JsonShelf = deepcopy(shelf)
		JsonShelf['Name'] = JsonShelf['Name'] + '_{:03d}'.format(cnt + 1)
		JsonShelf['id'] = cnt
		JsonShelves.append(JsonShelf)

JsonProducts = []
for productgroup in productgroups:
	JsonProductGroup = deepcopy(productgroup)
	JsonProductGroup['Members'] = []
	for cnt in range(productgroup['Repeat']):
		JsonProductGroup['Members'].append({
			'id': cnt,
			'Name': JsonProductGroup['GroupName'] + '_{:03d}'.format(cnt + 1),
			'Scale': JsonProductGroup['Scale']
		})
	JsonProducts.append(JsonProductGroup)

JsonRoot['Whitelist']['Shelves'] = JsonShelves
JsonRoot['Whitelist']['Products'] = JsonProducts
with open(dstjsonname, 'w') as f:
	json.dump(JsonRoot, f)
