import onnx
from onnx import helper
from onnx import AttributeProto, TensorProto, GraphProto


default_alpha = 0.01
node = onnx.helper.make_node(
    'LReLU_TRT',
    inputs=['x'],
    outputs=['y'],
)

# Create one input (ValueInfoProto)
x = helper.make_tensor_value_info('x', TensorProto.FLOAT, [3, 4, 5])

# Create one output (ValueInfoProto)
y = helper.make_tensor_value_info('y', TensorProto.FLOAT, [3, 4, 5])

# Create the graph (GraphProto)
graph_def = helper.make_graph(
    [node],
    'test-model',
    [x],
    [y],
)

# Create the model (ModelProto)
model_def = helper.make_model(graph_def, producer_name='onnx-example')
model_def.opset_import[0].version = 13
model_def.ir_version = 7
model_def = onnx.shape_inference.infer_shapes(model_def)
print('The model is:\n{}'.format(model_def))
# onnx.checker.check_model(model_def)
print('The model is checked!')

onnx.save(model_def, "leakyrelu.onnx")
