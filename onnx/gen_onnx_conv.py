import onnx
from onnx import helper
from onnx import AttributeProto, TensorProto, GraphProto
import numpy as np


X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1, 128, 28, 28])
Y = helper.make_tensor_value_info('Y', TensorProto.FLOAT, [1, 128, 28, 28])
Y1 = helper.make_tensor_value_info('Y1', TensorProto.FLOAT, [1, 128, 28, 28])

W = helper.make_tensor_value_info('W', TensorProto.FLOAT, [128, 128, 3, 3])


# Convolution with strides=2 and padding
node_with_padding = onnx.helper.make_node(
    'Conv',
    inputs=['X', 'W'],
    outputs=['Y'],
    kernel_shape=[3, 3],
    pads=[1, 1, 1, 1],
    strides=[1, 1],  # Default values for other attributes: dilations=[1, 1]
    group=1
)
node_with_padding2 = onnx.helper.make_node(
    'Conv',
    inputs=['Y', 'W'],
    outputs=['Y1'],
    kernel_shape=[3, 3],
    pads=[1, 1, 1, 1],
    strides=[1, 1],  # Default values for other attributes: dilations=[1, 1]
    group=1
)
# Create the graph (GraphProto)
graph_def = helper.make_graph(
    [node_with_padding, node_with_padding2],
    'test-model',
    [X, W],
    [Y],
)

# Create the model (ModelProto)
model_def = helper.make_model(graph_def, producer_name='onnx-example')
model_def.opset_import[0].version = 13
model_def.ir_version = 7
model_def = onnx.shape_inference.infer_shapes(model_def)
print('The model is:\n{}'.format(model_def))
onnx.checker.check_model(model_def)
print('The model is checked!')

onnx.save(model_def, "conv.onnx")
