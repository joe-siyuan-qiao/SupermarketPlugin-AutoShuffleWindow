# This script automatically generate Whitelist.json
# See the script to know how to use this

from copy import deepcopy
from random import shuffle
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
    'GroupName': 'chips_pringles',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 501),
    'Scale': 1.0,
    'Discard': False
})

productgroups.append({
    'GroupName': 'chips_can_lays',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': [1, 2, 7, 8, 11, 12, 15, 16, 19, 20, 21, 22, 25, 26, 29, 30, 33, 34, 39, 40],
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Green can'
})

productgroups.append({
    'GroupName': 'chips_can_lays',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': [3, 6, 9, 13, 17, 24, 28, 31, 38],
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Yellow can'
})

productgroups.append({
    'GroupName': 'chips_can_lays',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': [4, 5, 10, 14, 18, 23, 27, 32, 37],
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Dark red can'
})

productgroups.append({
    'GroupName': 'chips_barbaras',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 31),
    'Scale': 1.0,
    'Discard': False
})


productgroups.append({
    'GroupName': 'chips_box_olddutch',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 43),
    'Scale': 1.0,
    'Discard': False
})



productgroups.append({
    'GroupName': 'chips_cretos',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 31),
    'Scale': 1.0,
    'Discard': False
})

productgroups.append({
    'GroupName': 'chips_cyclone',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 11) + range(57, 67),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Blue and white'
})

productgroups.append({
    'GroupName': 'chips_cyclone',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(11, 57),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Yellow'
})

productgroups.append({
    'GroupName': 'chips_doritos',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 11) + range(21, 24) + range(28, 32) + [37, 39, 40] + range(50, 99) + [104, 105, 107] + range(112, 116) + range(120, 123) + range(132, 142),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Red'
})

productgroups.append({
    'GroupName': 'chips_doritos',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(11, 21) + range(123, 132),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Dark'
})

productgroups.append({
    'GroupName': 'chips_doritos',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(24, 28) + range(32, 37) + [38, ] + range(42, 50) + range(99, 104) + [106, ] + range(108, 112) + range(116, 120),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Green'
})

productgroups.append({
    'GroupName': 'chips_kettle',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(1, 11) + range(168, 177),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Blue'
})

productgroups.append({
    'GroupName': 'chips_kettle',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(11, 21) + range(158, 168),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Red'
})

productgroups.append({
    'GroupName': 'chips_kettle',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': [21, 22, 24, 26, 31, 32, 33, 34, 39, 43, 45, 46, 48] + range(50, 54) + range(58, 121) + range(125, 129) + [133, 135, 137, 138, 139]  + range(144, 148) + [152, 154, 156, 157],
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Dark'
})

productgroups.append({
    'GroupName': 'chips_kettle',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': [23, 25, 27, 28, 29, 30] + range(35, 39) + range(140, 144) + range(148, 152) + [153, 155],
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Yellow'
})

productgroups.append({
    'GroupName': 'chips_kettle',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': [42, 44, 47, 49,] + range(54, 58) + range(121, 125) + range(129, 133) + [134, 136],
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Orange'
})

productgroups.append({
    'GroupName': 'chips_lays',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(1, 235),
    'Scale': 1.0,
    'Discard': False
})

productgroups.append({
    'GroupName': 'chips_sun',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(1, 235),
    'Scale': 1.0,
    'Discard': False
})

productgroups.append({
    'GroupName': 'chips_raffles',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(1, 11) + range(48, 58),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'White and blue'
})

productgroups.append({
    'GroupName': 'chips_raffles',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(11, 20),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Red'
})

productgroups.append({
    'GroupName': 'chips_raffles',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(39, 47),
    'Scale': 1.0,
    'Discard': False,
    'Description': 'Dark'
})

productgroups.append({
    'GroupName': 'chips_pringles_small',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(1, 501),
    'Scale': 1.0,
    'Discard': False
})

productgroups.append({
    'GroupName': 'chips_olddutch',
    'ShelfName': 'BP_ShelfMain_001',
    'Repeat': range(1, 1001),
    'Scale': 1.0,
    'Discard': False
})

productgroups.append({
    'GroupName': 'chips_box_lays',
    'ShelfName': 'BP_ShelfMain_002',
    'Repeat': range(1, 31),
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
    repeatgroup = productgroup['Repeat']
    shuffle(repeatgroup)
    for cnt in repeatgroup:
        JsonProductGroup['Members'].append({
            'id': cnt,
            'Name': JsonProductGroup['GroupName'] + '_{:03d}'.format(cnt),
            'Scale': JsonProductGroup['Scale']
        })
    JsonProducts.append(JsonProductGroup)

JsonRoot['Whitelist']['Shelves'] = JsonShelves
JsonRoot['Whitelist']['Products'] = JsonProducts
with open(dstjsonname, 'w') as f:
    json.dump(JsonRoot, f)
