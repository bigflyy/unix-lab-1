from fastapi import FastAPI
import pika
import json
import uuid

app = FastAPI()

# подключение к RabbitMQ
connection = pika.BlockingConnection(pika.ConnectionParameters(host='rabbitmq'))
channel = connection.channel()
channel.queue_declare(queue='nlp_tasks')

@app.post("/embed/")
def embed_text(text: str):
    task_id = str(uuid.uuid4())
    message = {"id": task_id, "text": text}
    channel.basic_publish(exchange='', routing_key='nlp_tasks', body=json.dumps(message))
    return {"task_id": task_id, "status": "queued"}
